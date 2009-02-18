// __BEGIN_LICENSE__
// Copyright (C) 2006, 2007 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


#include <vw/FileIO.h>

using namespace vw;

typedef PixelRGBA<int> PixelT;
typedef char CharT;
typedef char _CharT;
typedef ImageView<PixelT> ImageT;
typedef ImageT ElemT;

#include "TestInstantiateFreeList.hh"

class TestInstantiateFileIOFree : public CxxTest::TestSuite
{
  public: void test_inst() {}
};