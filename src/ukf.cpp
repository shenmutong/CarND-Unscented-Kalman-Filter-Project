#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 30;
  //std_a_ = 0.2;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 30;
  //std_yawdd_ = 0.2;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;
  
  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;
  
  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_  = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.
  
  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
  is_initialized_ = false;
  //set state dimension
  n_x_ = 5;
  //set augemnted state dimension
  n_aug_ = 7;
  //set lambda
  lambda_ = 3 -n_aug_;
  //init sigma points matrix
  Xsig_pred_ = MatrixXd(n_x_, 2 *n_aug_ +1);
  //sigma points weights
  weights_ = VectorXd(2 * n_aug_+1);
  //initialize P 
  P_ <<     0.0043,   -0.0013,    0.0030,   -0.0022,   -0.0020,
    -0.0013,    0.0077,    0.0011,    0.0071,    0.0060,
    0.0030,    0.0011,    0.0054,    0.0007,    0.0008,
    -0.0022,    0.0071,    0.0007,    0.0098,    0.0100,
    -0.0020,    0.0060,    0.0008,    0.0100,    0.0123;
  
  R_Radar_ = MatrixXd(3,3);
  R_Radar_ <<
    std_radr_*std_radr_, 0, 0,
    0, std_radphi_*std_radphi_, 0,
    0, 0,std_radrd_*std_radrd_;
  R_Ladar_ = MatrixXd(2,2);
  R_Ladar_ << std_laspx_ * std_laspx_,0,
    0,std_laspy_* std_laspy_;

  // initial weights
  double weight_0 = lambda_ /( lambda_ + n_aug_);
  weights_(0) = weight_0;
  for (int i=1; i<2 * n_aug_+1; i++) {
    double weight = 0.5/(n_aug_ + lambda_);
    weights_(i) = weight;
  }
}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**  
  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
  
  if(!is_initialized_){
    //initialize
    //init data
    cout<< "INITIALIZE STEP" << endl;
    if(meas_package.sensor_type_ == MeasurementPackage::RADAR){
      // convert radar measurement data to x_
      double rho = meas_package.raw_measurements_[0];
      double v =  meas_package.raw_measurements_[1];
      double phi = meas_package.raw_measurements_[2];
      double p_x = rho * cosf(phi);
      double p_y = rho * sinf(phi);
      
      x_ <<  p_x ,p_y,v,phi,0;
      
    }else{
      x_ << meas_package.raw_measurements_[0],meas_package.raw_measurements_[1],4,0,0;
    }
    cout << "x_ = " << x_ <<endl;
    time_us_ = meas_package.timestamp_;
    is_initialized_ = true;
    return;
  }
  
  //check meas_package
  if(meas_package.sensor_type_ == MeasurementPackage::RADAR && !use_radar_ ){
    return;
  }
  if(meas_package.sensor_type_ == MeasurementPackage::LASER && !use_laser_ ){
    return;
  }
  
  //clac dt
  double dt = (meas_package.timestamp_ - time_us_) / 1000000.0;	//dt - expressed in seconds
  cout <<"delta t" << dt <<endl;
  time_us_ = meas_package.timestamp_;
  //PREDICT STEP
  cout<< "PREDICT START"<< endl;
  Prediction(dt);
  cout<< "PREDICT END"<< endl;
  
  
  if(meas_package.sensor_type_ == MeasurementPackage::RADAR){
     UpdateRadar(meas_package);
  }else{
    UpdateLidar(meas_package);
  }
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
  
  //get sigma points
  //generating Sigma points
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_+1);
  //augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);
  //augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
  x_aug.head(5) = x_;
  x_aug(5) =0;
  x_aug(6) = 0;
  P_aug.fill(0);
  P_aug.topLeftCorner(5,5) = P_;
  P_aug(5,5) = std_a_;
  P_aug(6,6) = std_yawdd_;
  MatrixXd A = P_aug.llt().matrixL();
  //Xsig_pred_ =
  Xsig_aug.col(0) = x_aug;
  for (int i = 0; i < n_aug_; i++) {
    Xsig_aug.col(i+1)     = x_aug + sqrt(lambda_ + n_aug_) * A.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_ + n_aug_) * A.col(i);
    }
  // cout<< "Xsig_aug"<<Xsig_aug <<endl;

  //Augmentation Assignment predict Sigma points  
  SigmaPointPrediction(Xsig_aug,delta_t);
  
  //predicted Mean and Covariance
  PredictMeanAndConvariance();
  /**/
}


/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
  
  MatrixXd Zsig = MatrixXd(2,2 * n_aug_ +1);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    // extract values for better readibility
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    // measurement model
    Zsig(0,i) =Xsig_pred_(0,i);                        //px
    Zsig(1,i) = Xsig_pred_(1,i);                                 //py
  }
  UpdateCommon(Zsig, meas_package.raw_measurements_);
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:
  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
  //transform sigma points into measurement space
  int n_z = 3;
  MatrixXd Zsig = MatrixXd(n_z,2 * n_aug_ +1);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    // extract values for better readibility
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //rho
    Zsig(1,i) = atan2(p_y,p_x);                                 //phi
    Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //rho_dot
  }
  UpdateCommon(Zsig, meas_package.raw_measurements_);


  
  
}
void UKF::UpdateCommon(MatrixXd Zsig,Eigen::VectorXd z){
  int n_z = Zsig.rows();
  //VectorXd z_pred = VectorXd(3);
  VectorXd z_pred = VectorXd(n_z);// = meas_package.raw_measurements_;
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; i++) {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  
  //innovation covariance matrix S
  
  MatrixXd S = MatrixXd(n_z,n_z);
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points
    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }

  //add measurement noise covariance matrix
  //MatrixXd R = MatrixXd(n_z,n_z);
  //R <<    std_radr*std_radr, 0, 0,
  //       0, std_radphi*std_radphi, 0,
  //       0, 0,std_radrd*std_radrd;
  if(n_z >2){
    S = S + R_Radar_;
  }else{
    S = S + R_Ladar_;
  }
  

  
  MatrixXd Tc = MatrixXd(n_x_,n_z);
  //calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //2n+1 simga points

    //residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    
      //angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
      // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
      //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;
    


    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  //Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  //residual
  VectorXd z_diff =z  - z_pred;

  cout << "z_diff " << z_diff <<endl;

  //angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
  
  //update state mean and covariance matrix
  x_ = x_ + K * z_diff;
  P_ = P_ - K*S*K.transpose();
  cout<< "Update" << endl;
  cout << "x_ =" << x_ <<endl;
  cout << "P_ ="<< P_ <<endl;
  /**/
}

void UKF::SigmaPointPrediction(MatrixXd xsig_aug,double delta_t){
  for(int i =0; i<2 *n_aug_ +1; i++ ){
    //extract values for better readability
    double p_x = xsig_aug(0,i);
    double p_y = xsig_aug(1,i);
    double v = xsig_aug(2,i);
    double yaw = xsig_aug(3,i);
    double yawd = xsig_aug(4,i);
    double nu_a = xsig_aug(5,i);
    double nu_yawdd = xsig_aug(6,i);

    double px_p ,py_p;

    if (fabs(yawd) > 0.001) {
      px_p = p_x + v/yawd * ( sin (yaw + yawd* delta_t) - sin(yaw));
      py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd * delta_t) );
    }
    else {
      px_p = p_x + v* delta_t * cos(yaw);
      py_p = p_y + v* delta_t * sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd * delta_t;
    double yawd_p = yawd;

    double dt_2 = delta_t * delta_t;

    //add noise
    px_p = px_p + 0.5* nu_a * dt_2 * cos(yaw);
    py_p = py_p + 0.5* nu_a * dt_2 * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5* nu_yawdd * dt_2;
    yawd_p = yawd_p + nu_yawdd * delta_t;

    //write predicted sigma point into right column
    Xsig_pred_(0,i) = px_p;
    Xsig_pred_(1,i) = py_p;
    Xsig_pred_(2,i) = v_p;
    Xsig_pred_(3,i) = yaw_p;
    Xsig_pred_(4,i) = yawd_p;

  }
  //cout<< "Xsig_Pred_"<<Xsig_pred_ <<endl;
}
void UKF::PredictMeanAndConvariance(){
  

 
  //cout << "weights" <<weights_<< endl;

  //predicted state mean
  x_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {
    x_ = x_+ weights_(i) * Xsig_pred_.col(i);
  }
  cout <<"x_ = "  << x_ << endl;
  cout <<"Xsig " << Xsig_pred_<<endl;

  //predicted state covariance matrix
  P_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    cout<< "x_diff = " << x_diff(3) <<endl;
    //angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    P_ = P_ + weights_(i) * x_diff * x_diff.transpose() ;
  
  }
  cout <<"P_ = "  << P_ << endl;
}

