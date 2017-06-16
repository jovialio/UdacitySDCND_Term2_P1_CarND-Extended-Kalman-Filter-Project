#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;


/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);
  
  //measurement covariance/noise matrix - laser
  R_laser_ << 0.0225, 0,
        0, 0.0225;

  //measurement covariance/noise matrix - radar
  R_radar_ << 0.09, 0, 0,
              0, 0.0009, 0,
              0, 0, 0.09;

  /*
  TODO: Initialize FusionEKF Matrix.
  */

  //initialize H_laser measurement matrix
  H_laser_ << 1, 0, 0, 0,
            0, 1, 0, 0;
  
  //state covariance matrix P_
  ekf_.P_ = MatrixXd(4, 4);
  ekf_.P_ << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1000, 0,
            0, 0, 0, 1000;
    
    
  //initial transition matrix F_
  ekf_.F_ = MatrixXd(4, 4);
  ekf_.F_ << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;

  //set the acceleration noise components
  noise_ax = 9;
  noise_ay = 9;

}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {
    

  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
    TODO:
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      
      //initialize state x_ using radar measurement (polar) coverted to cartesian
      ekf_.x_ << measurement_pack.raw_measurements_[0] * cos(measurement_pack.raw_measurements_[1]), 
                measurement_pack.raw_measurements_[0] * sin(measurement_pack.raw_measurements_[1]), 
                measurement_pack.raw_measurements_[2] * cos(measurement_pack.raw_measurements_[1]), 
                measurement_pack.raw_measurements_[2] * sin(measurement_pack.raw_measurements_[1]);

      //update measurement covariance/noise using radar noise
      ekf_.R_ = MatrixXd(3, 3);
      ekf_.R_ = R_radar_;

      //update measurement matrix
      ekf_.H_ = MatrixXd(3, 4);
      ekf_.H_ << tools.CalculateJacobian(ekf_.x_);
        

    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */

      //initialize state x_ using laser measurement and setting vx and vy to 0
      ekf_.x_ << measurement_pack.raw_measurements_[0], measurement_pack.raw_measurements_[1], 0, 0;

      //update measurement covariance/noise using laser noise
      ekf_.R_ = MatrixXd(2, 2);
      ekf_.R_ = R_laser_;

      //update measurement matrix
      ekf_.H_ = MatrixXd(2, 4);
      ekf_.H_ << H_laser_;

    }

    // done initializing, no need to predict or update
    previous_timestamp_ = measurement_pack.timestamp_;
    is_initialized_ = true;
    return;
  }
    
    

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
   TODO:
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
   */
    
  //compute the time elapsed between the current and previous measurements
  float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0; //dt - expressed in seconds
 
  previous_timestamp_ = measurement_pack.timestamp_;
  cout << dt << endl;
    
  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_3 * dt;
    
  //Modify the F matrix so that the time is integrated
  ekf_.F_(0,2) = dt;
  ekf_.F_(1,3) = dt;
    
  //Set the process covariance matrix Q
  ekf_.Q_ = MatrixXd(4, 4);
    
  ekf_.Q_ <<  dt_4/4*noise_ax, 0, dt_3/2*noise_ax, 0,
                0, dt_4/4*noise_ay, 0, dt_3/2*noise_ay,
                dt_3/2*noise_ax, 0, dt_2*noise_ax, 0,
                0, dt_3/2*noise_ay, 0, dt_2*noise_ay;

  ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
   TODO:
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
      
    //update measurement covariance/noise using radar noise
    ekf_.R_ = MatrixXd(3, 3);
    ekf_.R_ = R_radar_;

    //update measurement matrix
    ekf_.H_ = MatrixXd(3, 4);
    ekf_.H_ << tools.CalculateJacobian(ekf_.x_);
    
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } 
  else {
    //update measurement covariance/noise using laser noise
    ekf_.R_ = MatrixXd(2, 2);
    ekf_.R_ = R_laser_;

    //update measurement matrix
    ekf_.H_ = MatrixXd(2, 4);
    ekf_.H_ << H_laser_;
    
    ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
