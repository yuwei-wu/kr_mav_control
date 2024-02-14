#ifndef QUADROTOR_SIMULATOR_BASE_HPP_
#define QUADROTOR_SIMULATOR_BASE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <rcpputils/asserts.hpp>

#include <geometry_msgs/msg/vector3_stamped.hpp>
#include <kr_mav_msgs/msg/output_data.hpp>
#include <kr_quadrotor_simulator_humble/Quadrotor.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <tf2_ros/transform_broadcaster.h> 

#include <Eigen/Geometry>

namespace QuadrotorSimulator
{
template <typename T, typename U>
class QuadrotorSimulatorBase : public rclcpp::Node
{
 public:
  QuadrotorSimulatorBase(const rclcpp::NodeOptions & options);
  void run(void);
  // void extern_force_callback(const geometry_msgs::Vector3Stamped::ConstPtr &f_ext);
  // void extern_moment_callback(const geometry_msgs::Vector3Stamped::ConstPtr &m_ext);

 protected:
  typedef struct _ControlInput
  {
    double rpm[4];
  } ControlInput;

  /*
   * The callback called when ROS message arrives and needs to fill in the
   * command_ field that would be passed to the getControl function.
   */
  virtual void cmd_callback(const typename T::ConstPtr &cmd) = 0;

  /*
   * Called by the simulator to get the current motor speeds. This is the
   * controller that would be running on the robot.
   * @param[in] quad Quadrotor instance which is being simulated
   * @param[in] cmd The command input which is filled by cmd_callback
   */
  virtual ControlInput getControl(const Quadrotor &quad, const U &cmd) const = 0;

  Quadrotor quad_;
  U command_;

 private:
  void stateToOdomMsg(const Quadrotor::State &state, nav_msgs::msg::Odometry &odom) const;
  void quadToImuMsg(const Quadrotor &quad, sensor_msgs::msg::Imu &imu) const;
  void tfBroadcast(const nav_msgs::msg::Odometry &odom_msg);

  // ROS1: 
  // ros::Publisher pub_odom_;
  // ros::Publisher pub_imu_;
  // ros::Publisher pub_output_data_;
  // ros::Subscriber sub_cmd_;
  // ros::Subscriber sub_extern_force_;
  // ros::Subscriber sub_extern_moment_;

  rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr pub_odom_;
  rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr pub_imu_;
  rclcpp::Publisher<kr_mav_msgs::msg::OutputData>::SharedPtr pub_output_data_;
  typename rclcpp::Subscription<T>::SharedPtr sub_cmd_;
  rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr sub_extern_force_;
  rclcpp::Subscription<geometry_msgs::msg::Vector3Stamped>::SharedPtr sub_extern_moment_;
  double simulation_rate_;
  double odom_rate_;
  std::string quad_name_;
  std::string world_frame_id_;

  tf2_ros::TransformBroadcaster tf_broadcaster_;
};

template <typename T, typename U>
QuadrotorSimulatorBase<T, U>::QuadrotorSimulatorBase(const rclcpp::NodeOptions & options)
{
  pub_odom_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 100);
  pub_imu_ = this->create_publisher<sensor_msgs::msg::Imu>("imu", 100);
  pub_output_data_ = this->create_publisher<kr_mav_msgs::msg::OutputData>("output_data", 100);
  sub_cmd_ = this->create_subscription<T>(
      "cmd", 100, std::bind(&QuadrotorSimulatorBase::cmd_callback, this, std::placeholders::_1));
  sub_extern_force_ = this->create_subscription<geometry_msgs::msg::Vector3Stamped>(
      "extern_force", 10, std::bind(&QuadrotorSimulatorBase::extern_force_callback, this, std::placeholders::_1));
  sub_extern_moment_ = this->create_subscription<geometry_msgs::msg::Vector3Stamped>(
      "extern_moment", 10, std::bind(&QuadrotorSimulatorBase::extern_moment_callback, this, std::placeholders::_1));

  // ROS1: 
  // pub_odom_ = n.advertise<nav_msgs::Odometry>("odom", 100);
  // pub_imu_ = n.advertise<sensor_msgs::Imu>("imu", 100);
  // pub_output_data_ = n.advertise<kr_mav_msgs::OutputData>("output_data", 100);
  // sub_cmd_ =
  //     n.subscribe<T>("cmd", 100, &QuadrotorSimulatorBase::cmd_callback, this, ros::TransportHints().tcpNoDelay());
  // sub_extern_force_ = n.subscribe<geometry_msgs::Vector3Stamped>(
  //     "extern_force", 10, &QuadrotorSimulatorBase::extern_force_callback, this, ros::TransportHints().tcpNoDelay());
  // sub_extern_moment_ = n.subscribe<geometry_msgs::Vector3Stamped>(
  //     "extern_moment", 10, &QuadrotorSimulatorBase::extern_moment_callback, this, ros::TransportHints().tcpNoDelay());

  declare_parameter("rate/simulation", 1000.0);
  get_parameter("rate/simulation", simulation_rate_);
  rcpputils::assert_true(simulation_rate_ > 0);

  declare_parameter("rate/odom", 100.0);
  get_parameter("rate/odom", odom_rate_);

  declare_parameter("world_frame_id", std::string("simulator"));
  get_parameter("world_frame_id", world_frame_id_);

  declare_parameter("quadrotor_name", std::string("quadrotor"));
  get_parameter("quadrotor_name", quad_name_);

  // ROS1: 
  // n.param("rate/simulation", simulation_rate_, 1000.0);
  // n.param("rate/odom", odom_rate_, 100.0);
  // n.param("world_frame_id", world_frame_id_, std::string("simulator"));
  // n.param("quadrotor_name", quad_name_, std::string("quadrotor"));



  // TODO: Convert this lambda function. Use list_parameters and search through 
  // list for desired param.
  
  // auto get_param = [&n](const std::string &param_name) {
  //   double param;
  //   if(!n.hasParam(param_name))
  //   {
  //     ROS_WARN("Simulator sleeping to wait for param %s", param_name.c_str());
  //     ros::Duration(0.5).sleep();
  //   }
  //   if(!n.getParam(param_name, param))
  //   {
  //     const std::string error_msg = param_name + " not set";
  //     ROS_FATAL_STREAM(error_msg);
  //     throw std::logic_error(error_msg);
  //   }
  //   return param;
  // };
  // END TODO #################################################################

  quad_.setMass(get_parameter("mass").as_double());
  quad_.setInertia(Eigen::Vector3d(get_parameter("Ixx").as_double(), get_parameter("Iyy").as_double(), get_parameter("Izz").as_double()).asDiagonal());
  quad_.setGravity(get_parameter("gravity").as_double());
  quad_.setPropRadius(get_parameter("prop_radius").as_double());
  quad_.setPropellerThrustCoefficient(get_parameter("thrust_coefficient").as_double());
  quad_.setArmLength(get_parameter("arm_length").as_double());
  quad_.setMotorTimeConstant(get_parameter("motor_time_constant").as_double());
  quad_.setMinRPM(get_parameter("min_rpm").as_double());
  quad_.setMaxRPM(get_parameter("max_rpm").as_double());
  quad_.setDragCoefficient(get_parameter("drag_coefficient").as_double());

  Eigen::Vector3d initial_pos;

  declare_parameter("initial_position/x", 0.0);
  declare_parameter("initial_position/y", 0.0);
  declare_parameter("initial_position/z", 0.0);
  get_parameter("initial_position/x", initial_pos(0));
  get_parameter("initial_position/y", initial_pos(1));
  get_parameter("initial_position/z", initial_pos(2));

  // ROS1:
  // n.param("initial_position/x", initial_pos(0), 0.0);
  // n.param("initial_position/y", initial_pos(1), 0.0);
  // n.param("initial_position/z", initial_pos(2), 0.0);

  Eigen::Quaterniond initial_q;
  declare_parameter("initial_orientation/w", 1.0);
  declare_parameter("initial_orientation/x", 0.0);
  declare_parameter("initial_orientation/y", 0.0);
  declare_parameter("initial_orientation/z", 0.0);
  get_parameter("initial_orientation/w", initial_q.w());
  get_parameter("initial_orientation/x", initial_q.x());
  get_parameter("initial_orientation/y", initial_q.y());
  get_parameter("initial_orientation/z", initial_q.z());

  // ROS1:
  // n.param("initial_orientation/w", initial_q.w(), 1.0);
  // n.param("initial_orientation/x", initial_q.x(), 0.0);
  // n.param("initial_orientation/y", initial_q.y(), 0.0);
  // n.param("initial_orientation/z", initial_q.z(), 0.0);

  initial_q.normalize();
  Quadrotor::State state = quad_.getState();
  state.x(0) = initial_pos(0);
  state.x(1) = initial_pos(1);
  state.x(2) = initial_pos(2);
  state.R = initial_q.matrix();
  quad_.setState(state);
}
 
template <typename T, typename U>
void QuadrotorSimulatorBase<T, U>::run(void)
{
  // Call once with empty command to initialize values
  cmd_callback(std::make_shared<T>());

  QuadrotorSimulatorBase::ControlInput control;

  nav_msgs::msg::Odometry odom_msg;
  sensor_msgs::msg::Imu imu_msg;
  kr_mav_msgs::msg::OutputData output_data_msg;

  odom_msg.header.frame_id = world_frame_id_;
  odom_msg.child_frame_id = quad_name_;
  imu_msg.header.frame_id = quad_name_;
  output_data_msg.header.frame_id = quad_name_;

  const double simulation_dt = 1 / simulation_rate_;

  // ros::Rate r(simulation_rate_);
  rclcpp::Rate r(simulation_rate_);

  // const ros::Duration odom_pub_duration(1 / odom_rate_);
  const rclcpp::Duration odom_pub_duration(1 / odom_rate_);
  rclcpp::Time next_odom_pub_time = this->now();

  auto base_node = std::make_shared<this>();
  while(rclcpp::ok())
  {
    rclcpp::spin(base_node);


    control = getControl(quad_, command_);
    quad_.setInput(control.rpm[0], control.rpm[1], control.rpm[2], control.rpm[3]);
    quad_.step(simulation_dt);

    rclcpp::Time tnow = this->now();

    if(tnow >= next_odom_pub_time)
    {
      next_odom_pub_time += odom_pub_duration;
      const Quadrotor::State &state = quad_.getState();

      stateToOdomMsg(state, odom_msg);
      odom_msg.header.stamp = tnow;
      pub_odom_->publish(odom_msg);
      tfBroadcast(odom_msg);

      quadToImuMsg(quad_, imu_msg);
      imu_msg.header.stamp = tnow;
      pub_imu_->publish(imu_msg);

      // Also publish an OutputData msg
      output_data_msg.header.stamp = tnow;
      output_data_msg.orientation = imu_msg.orientation;
      output_data_msg.angular_velocity = imu_msg.angular_velocity;
      output_data_msg.linear_acceleration = imu_msg.linear_acceleration;
      output_data_msg.motor_rpm[0] = state.motor_rpm(0);
      output_data_msg.motor_rpm[1] = state.motor_rpm(1);
      output_data_msg.motor_rpm[2] = state.motor_rpm(2);
      output_data_msg.motor_rpm[3] = state.motor_rpm(3);
      pub_output_data_->publish(output_data_msg);
    }

    r.sleep();
  }
}
// 
// template <typename T, typename U>
// void QuadrotorSimulatorBase<T, U>::extern_force_callback(const geometry_msgs::Vector3Stamped::ConstPtr &f_ext)
// {
//   quad_.setExternalForce(Eigen::Vector3d(f_ext->vector.x, f_ext->vector.y, f_ext->vector.z));
// }
// 
// template <typename T, typename U>
// void QuadrotorSimulatorBase<T, U>::extern_moment_callback(const geometry_msgs::Vector3Stamped::ConstPtr &m_ext)
// {
//   quad_.setExternalMoment(Eigen::Vector3d(m_ext->vector.x, m_ext->vector.y, m_ext->vector.z));
// }
// 
template <typename T, typename U>
void QuadrotorSimulatorBase<T, U>::stateToOdomMsg(const Quadrotor::State &state, nav_msgs::msg::Odometry &odom) const
{
  odom.pose.pose.position.x = state.x(0);
  odom.pose.pose.position.y = state.x(1);
  odom.pose.pose.position.z = state.x(2);

  Eigen::Quaterniond q(state.R);
  odom.pose.pose.orientation.x = q.x();
  odom.pose.pose.orientation.y = q.y();
  odom.pose.pose.orientation.z = q.z();
  odom.pose.pose.orientation.w = q.w();

  odom.twist.twist.linear.x = state.v(0);
  odom.twist.twist.linear.y = state.v(1);
  odom.twist.twist.linear.z = state.v(2);

  odom.twist.twist.angular.x = state.omega(0);
  odom.twist.twist.angular.y = state.omega(1);
  odom.twist.twist.angular.z = state.omega(2);
}
// 
template <typename T, typename U>
void QuadrotorSimulatorBase<T, U>::quadToImuMsg(const Quadrotor &quad, sensor_msgs::msg::Imu &imu) const
{
  const Quadrotor::State state = quad.getState();
  Eigen::Quaterniond q(state.R);
  imu.orientation.x = q.x();
  imu.orientation.y = q.y();
  imu.orientation.z = q.z();
  imu.orientation.w = q.w();

  imu.angular_velocity.x = state.omega(0);
  imu.angular_velocity.y = state.omega(1);
  imu.angular_velocity.z = state.omega(2);

  const double kf = quad.getPropellerThrustCoefficient();
  const double m = quad.getMass();
  const Eigen::Vector3d &external_force = quad.getExternalForce();
  const double g = quad.getGravity();
  const double thrust = kf * state.motor_rpm.square().sum();
  Eigen::Vector3d acc;
  if(state.x(2) < 1e-4)
  {
    acc = state.R.transpose() * (external_force / m + Eigen::Vector3d(0, 0, g));
  }
  else
  {
    acc = thrust / m * Eigen::Vector3d(0, 0, 1) + state.R.transpose() * external_force / m;
    if(quad.getDragCoefficient() != 0)
    {
      const double drag_coefficient = quad.getDragCoefficient();
      const double mass = quad.getMass();
      Eigen::Matrix3d P;
      P << 1, 0, 0, 0, 1, 0, 0, 0, 0;
      acc -= drag_coefficient / mass * P * state.R.transpose() * state.v;
    }
  }

  imu.linear_acceleration.x = acc(0);
  imu.linear_acceleration.y = acc(1);
  imu.linear_acceleration.z = acc(2);
}
// 
template <typename T, typename U>
void QuadrotorSimulatorBase<T, U>::tfBroadcast(const nav_msgs::msg::Odometry &odom_msg)
{
  geometry_msgs::msg::TransformStamped ts;

  ts.header.stamp = odom_msg.header.stamp;
  ts.header.frame_id = odom_msg.header.frame_id;
  ts.child_frame_id = odom_msg.child_frame_id;

  ts.transform.translation.x = odom_msg.pose.pose.position.x;
  ts.transform.translation.y = odom_msg.pose.pose.position.y;
  ts.transform.translation.z = odom_msg.pose.pose.position.z;

  ts.transform.rotation = odom_msg.pose.pose.orientation;

  tf_broadcaster_.sendTransform(ts);
}
}  // namespace QuadrotorSimulator
 
#endif
