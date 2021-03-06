// __BEGIN_LICENSE__
// Copyright (C) 2006-2011 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


#include <gtest/gtest.h>
#include <vw/Math/Functions.h>

using namespace vw;

static const double DELTA = 1e-15;

TEST(Functions, ERF) {
  EXPECT_NEAR( -0.9999779095030014,    vw::math::impl::erf(-3.0)   , DELTA);
  EXPECT_NEAR( -0.8427007929497149,    vw::math::impl::erf(-1e0)   , DELTA);
  EXPECT_NEAR( -0.1124629160182849,    vw::math::impl::erf(-1e-1)  , DELTA);
  EXPECT_NEAR( -0.01128341555584962,   vw::math::impl::erf(-1e-2)  , DELTA);
  EXPECT_NEAR( -0.0001128379163334249, vw::math::impl::erf(-1e-4)  , DELTA);
  EXPECT_NEAR( 0,                      vw::math::impl::erf(0)      , DELTA);
  EXPECT_NEAR( 0.0001128379163334249,  vw::math::impl::erf(1e-4)   , DELTA);
  EXPECT_NEAR( 0.01128341555584962,    vw::math::impl::erf(1e-2)   , DELTA);
  EXPECT_NEAR( 0.1124629160182849,     vw::math::impl::erf(1e-1)   , DELTA);
  EXPECT_NEAR( 0.8427007929497149,     vw::math::impl::erf(1e0)    , DELTA);
  EXPECT_NEAR( 0.9999779095030014,     vw::math::impl::erf(3.0)    , DELTA);
}

TEST(Functions, ERFC) {
  EXPECT_NEAR( 1.999977909503001,      vw::math::impl::erfc(-3.0)  , DELTA);
  EXPECT_NEAR( 1.842700792949715,      vw::math::impl::erfc(-1e0)  , DELTA);
  EXPECT_NEAR( 1.112462916018285,      vw::math::impl::erfc(-1e-1) , DELTA);
  EXPECT_NEAR( 1.011283415555850,      vw::math::impl::erfc(-1e-2) , DELTA);
  EXPECT_NEAR( 1.000112837916333,      vw::math::impl::erfc(-1e-4) , DELTA);
  EXPECT_NEAR( 1,                      vw::math::impl::erfc(0)     , DELTA);
  EXPECT_NEAR( 0.9998871620836666,     vw::math::impl::erfc(1e-4)  , DELTA);
  EXPECT_NEAR( 0.9887165844441504,     vw::math::impl::erfc(1e-2)  , DELTA);
  EXPECT_NEAR( 0.8875370839817151,     vw::math::impl::erfc(1e-1)  , DELTA);
  EXPECT_NEAR( 0.1572992070502851,     vw::math::impl::erfc(1e0)   , DELTA);
  EXPECT_NEAR( 0.00002209049699858544, vw::math::impl::erfc(3.0)   , DELTA);
}
