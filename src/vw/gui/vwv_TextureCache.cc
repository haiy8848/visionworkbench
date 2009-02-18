// __BEGIN_LICENSE__
// Copyright (C) 2006, 2007 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


#include <vw/gui/vwv_TextureCache.h>

// --------------------------------------------------------------
//                     TextureRequest
// --------------------------------------------------------------

// Forward Declaration

struct TextureRequest {
  virtual void process_request() = 0;
};

// Allocate Texture Request 
class AllocateTextureRequest : public TextureRequest {
  boost::shared_ptr<TextureRecordBase> m_record;
  vw::ViewImageResource m_image;
  CachedTextureRenderer* m_parent;
public:
  AllocateTextureRequest( boost::shared_ptr<TextureRecordBase> texture_record, 
                          vw::ViewImageResource const image,
                          CachedTextureRenderer* parent) :
    m_record(texture_record), m_image(image), m_parent(parent) {}
  
  virtual void process_request() {
    m_record->texture_id = m_parent->allocate_texture(m_image);
  }
};

// Deallocate Texture Request 
class DeallocateTextureRequest : public TextureRequest {
  boost::shared_ptr<TextureRecordBase> m_record;
  CachedTextureRenderer* m_parent;
public:
  DeallocateTextureRequest( boost::shared_ptr<TextureRecordBase> texture_record,
                            CachedTextureRenderer* parent) :
    m_record(texture_record), m_parent(parent) {}
  
  virtual void process_request() {
    m_parent->deallocate_texture(m_record->texture_id);
    m_record->texture_id = 0;
  }
};

// --------------------------------------------------------------
//                     CachedTextureRenderer
// --------------------------------------------------------------

GLuint CachedTextureRenderer::request_allocation(boost::shared_ptr<TextureRecordBase> texture_record, 
                                                 vw::ViewImageResource const block) {
  vw::Mutex::Lock lock(m_incoming_request_mutex);
  m_incoming_requests.push_back( boost::shared_ptr<TextureRequest>(new AllocateTextureRequest(texture_record, block, this)) ); 
  m_needs_redraw = true;
}
void CachedTextureRenderer::request_deallocation(boost::shared_ptr<TextureRecordBase> texture_record) {
  vw::Mutex::Lock lock(m_incoming_request_mutex);
  m_incoming_requests.push_back( boost::shared_ptr<TextureRequest>(new DeallocateTextureRequest(texture_record, this)) );
  m_needs_redraw = true;
}
  
void CachedTextureRenderer::process_allocation_request() {
  vw::Mutex::Lock lock(m_incoming_request_mutex);
  
  boost::shared_ptr<TextureRequest> r;
  
  while (m_incoming_requests.size() != 0) {
    r = m_incoming_requests.front();
    m_incoming_requests.pop_front();
    r->process_request();
  }
}

// --------------------------------------------------------------
//                     TextureFetchTask
// --------------------------------------------------------------
  
class TextureFetchTask {
  bool terminate;
  vw::Mutex &m_outgoing_requests_mutex;
  std::list<boost::shared_ptr<TextureRecord> > &m_outgoing_requests;
  
public:
  TextureFetchTask(vw::Mutex &request_queue_mutex, 
                   std::list<boost::shared_ptr<TextureRecord> > &request_queue) : 
    terminate(false), m_outgoing_requests_mutex(request_queue_mutex), m_outgoing_requests(request_queue) {}
  
  void operator()() {
    while (!terminate) {
      
      // Drain the request queue
      bool found = false;
      boost::shared_ptr<TextureRecord> r;
      {
        vw::Mutex::Lock lock(m_outgoing_requests_mutex);
        if (m_outgoing_requests.size() != 0) {
          r = m_outgoing_requests.front();
          m_outgoing_requests.pop_front();
          found = true;
        } 
      }
      
      if (found) {
        // Force thet texture to regenerate.  Doing so will cause a
        // texture allocation request to be generated, and may cause
        // one or more texture deallocation requests to be produced as
        // well.
        r->texture_id = (*(r->handle)).get_texture_id();
      } else {

        // If there were no requests, sleep for a short time.
        vw::Thread::sleep_ms(100);
      }
    }
  }
  
  void kill() { terminate = true; }
};


// --------------------------------------------------------------
//                     GlTextureCache
// --------------------------------------------------------------

boost::shared_ptr<TextureRecord> GlTextureCache::get_record(vw::BBox2i bbox, int lod) {

  bool found = false;
  typedef std::vector<boost::shared_ptr<TextureRecord> >::iterator iter_type;
  
  // Smallest allowable LOD
  if (lod < 0) lod = 0;
  
  // Search for a matching texture record.
  iter_type best_match;
  iter_type iter = m_texture_records.begin();
  for (; iter != m_texture_records.end(); ++iter) {
    if ( (*iter)->bbox == bbox && (*iter)->lod <= lod) {
      
      if (!found) {
        best_match = iter;
        found = true;
      } else if ( (*iter)->lod > (*best_match)->lod) {
        best_match = iter;
      }
      
    }
  }
  if (found)
    return *best_match;
  else 
    vw::vw_throw(vw::LogicErr() << "GlTextureGenerator::get_record() failed.  No texture handle for bbox " << bbox << " @ LOD " << lod << ".");
  return boost::shared_ptr<TextureRecord>();  // never reached
}

GlTextureCache::GlTextureCache() {
  // Create the texture cache
  int gl_texture_cache_size = 128 * 1024 * 1024; // Use 128-MB of texture cache
  m_gl_texture_cache_ptr = new vw::Cache( gl_texture_cache_size );
  
  // Start the Texture Fetch thread
  m_texture_fetch_task.reset(new TextureFetchTask(m_outgoing_requests_mutex, m_outgoing_requests));
  m_texture_fetch_thread = new vw::Thread( m_texture_fetch_task );

  m_previous_lod = 0;
}

GlTextureCache::~GlTextureCache() {
  // Stop the Texture Fetch thread
  m_texture_fetch_task->kill();
  m_texture_fetch_thread->join();
  delete m_texture_fetch_thread;;
  
  // Free up remaining texture handles, and then the cache itself.
  m_outgoing_requests.clear();
  m_texture_records.clear();
  delete m_gl_texture_cache_ptr;
}

GLuint GlTextureCache::get_texture_id(vw::BBox2i bbox, int lod) {    
  boost::shared_ptr<TextureRecord> record = this->get_record(bbox, lod);
  
  // If the cache handle is invalid, we push the texture request
  // onto the stack.  The TextureFetchThread will regenerate this
  // texture and then send a message to the requestor to tell it to
  // retry drawing the screen.
  if ( record->texture_id == 0 ) {
    vw::vw_out(vw::VerboseDebugMessage) << "GlTextureCache::get_texture_id() missed bbox " 
                                        << bbox << " @ lod " << lod << ".  Generating.\n";
    vw::Mutex::Lock lock(m_outgoing_requests_mutex);

    // We purge the outgoing request queue whenever there is a change
    // in LOD so that we can immediately begin serving tiles at the
    // new level of detail.
    if (lod != m_previous_lod) {
      m_outgoing_requests.clear();
      m_previous_lod = lod;
    }

    m_outgoing_requests.push_back( record );
    return 0; 
  } else {
    vw::vw_out(vw::VerboseDebugMessage) << "GlTextureCache::get_texture_id() found bbox " 
                                        << bbox << " @ lod " << lod << ".\n";
    GLuint texture_id = record->texture_id;
    return texture_id;
  }
}
