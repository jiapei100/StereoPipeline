// __BEGIN_LICENSE__
// Copyright (C) 2006-2011 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__

#include <asp/Sessions/DG/StereoSessionDG.h>
#include <asp/Sessions/DG/XML.h>
#include <boost/scoped_ptr.hpp>
#include <test/Helpers.h>

#include <vw/Stereo/StereoModel.h>

using namespace vw;
using namespace asp;
using namespace xercesc;

TEST(StereoSessionDG, XMLReading) {
  XMLPlatformUtils::Initialize();

  GeometricXML geo;
  AttitudeXML att;
  EphemerisXML eph;
  ImageXML img;

  EXPECT_FALSE( geo.is_good() );
  EXPECT_FALSE( att.is_good() );
  EXPECT_FALSE( eph.is_good() );
  EXPECT_FALSE( img.is_good() );

  read_xml( "dg_example1.xml",
            geo, att, eph, img );

  EXPECT_TRUE( geo.is_good() );
  EXPECT_TRUE( att.is_good() );
  EXPECT_TRUE( eph.is_good() );
  EXPECT_TRUE( img.is_good() );

  // Checking GEO
  EXPECT_NEAR( 7949.165, geo.principal_distance, 1e-6 );
  EXPECT_EQ( 0, geo.optical_polyorder );
  EXPECT_VECTOR_NEAR( Vector3(), geo.perspective_center, 1e-6 );
  EXPECT_NEAR( 0, geo.camera_attitude.x(), 1e-6 );
  EXPECT_NEAR( 1, geo.camera_attitude.w(), 1e-6 );
  EXPECT_VECTOR_NEAR( Vector2(.05372,140.71193), geo.detector_origin, 1e-6 );
  EXPECT_NEAR( 0, geo.detector_rotation, 1e-6 );
  EXPECT_NEAR( .008, geo.detector_pixel_pitch, 1e-6 );

  // Checking ATT
  EXPECT_FALSE( att.start_time.empty() );
  EXPECT_NEAR( .02, att.time_interval, 1e-6 );
  EXPECT_EQ( 840, att.quat_vec.size() );
  EXPECT_EQ( 840, att.covariance_vec.size() );
  for ( size_t i = 0; i < 840; i++ ) {
    EXPECT_NE( 0, att.quat_vec[i].w() );
    EXPECT_NE( 0, att.covariance_vec[i][5] );
  }
  EXPECT_VECTOR_NEAR( Vector3(3.72e-12, 3.51e-12, 1.12e-13),
                      subvector(att.covariance_vec[0],0,3), 1e-20 );

  // Checking EPH
  EXPECT_FALSE( eph.start_time.empty() );
  EXPECT_NEAR( .02, eph.time_interval, 1e-6 );
  EXPECT_EQ( 840, eph.position_vec.size() );
  EXPECT_EQ( 840, eph.velocity_vec.size() );
  EXPECT_EQ( 840, eph.covariance_vec.size() );
  for ( size_t i = 0; i < 840; i++ ) {
    EXPECT_NE( 0, eph.position_vec[i].x() ) << i;
    EXPECT_NE( 0, eph.velocity_vec[i].x() );
    EXPECT_NE( 0, eph.covariance_vec[i][3] );
  }
  EXPECT_VECTOR_NEAR( Vector3(-1.150529111070304e+06,
                              -4.900037170821411e+06,
                              4.673402253879593e+06),
                      eph.position_vec[0], 1e-6 );

  // Checking IMG
  EXPECT_FALSE( img.tlc_start_time.empty() );
  EXPECT_FALSE( img.first_line_start_time.empty() );
  EXPECT_EQ( 2, img.tlc_vec.size() );
  EXPECT_NEAR( 0, img.tlc_vec[0].first, 1e-8 );
  EXPECT_NEAR( 0, img.tlc_vec[0].second, 1e-8 );
  EXPECT_NEAR( 23708, img.tlc_vec[1].first, 1e-8 );
  EXPECT_NEAR( 1.975667, img.tlc_vec[1].second, 1e-8 );
  EXPECT_EQ( 23708, img.image_size.y() );
  EXPECT_EQ( 35170, img.image_size.x() );

  XMLPlatformUtils::Terminate();
}

TEST(StereoSessionDG, CreateCamera) {
  StereoSessionDG session;

  boost::shared_ptr<camera::CameraModel> cam1( session.camera_model("", "dg_example1.xml") ),
    cam2( session.camera_model("", "dg_example2.xml") );
  ASSERT_TRUE( cam1.get() != 0 );
  ASSERT_TRUE( cam2.get() != 0 );

  Vector2 m1(13864, 5351), m2(15045, 5183);
  m1 *= 2; m2 *= 2;

  stereo::StereoModel sm(cam1.get(), cam2.get());
  double error;
  Vector3 pos = sm(m1,m2,error);
  EXPECT_LT( error, 5.0 ); // Triangulation should be better than 5 meters.
  EXPECT_LT( norm_2(pos), norm_2(cam1->camera_center(m1)) ); // Point should be below camera.
}