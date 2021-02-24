#ifndef ROBOT_H
#define ROBOT_H

#include <ros/ros.h>
#include <std_msgs/String.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/JointState.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <opencv2/core.hpp>
#include <tinyxml.h>
#include <random>

namespace map_simulator
{

struct Pose2D
{
  double x=0, y=0, theta=0;
};

class Robot
{
  enum class Shape{Cirle, Square};

  static char n_robots;
  static std::default_random_engine random_engine;
  static std::normal_distribution<double> unit_noise;
  char id;
  std::unique_ptr<ros::Subscriber> description_sub;
  std::unique_ptr<ros::Subscriber> cmd_sub;
  std::unique_ptr<ros::Publisher> odom_pub;
  std::unique_ptr<ros::Publisher> scan_pub;

  nav_msgs::Odometry odom;
  geometry_msgs::TransformStamped transform;

  // robot specs
  std::string robot_namespace;
  Shape shape;
  Pose2D pose;
  double linear_noise = 0, angular_noise = 0;
  // 2D laser offset / base_link
  Pose2D laser_pose;

  // optional publishers
  bool zero_joints = false;
  sensor_msgs::JointState joint_states;
  std::unique_ptr<ros::Publisher> js_pub;
  std::unique_ptr<tf2_ros::StaticTransformBroadcaster> static_tf_br;

  void loadModel(const std::string &urdf_xml, bool force_scanner, bool zero_joints, bool static_tf);

  template <typename T>
  static void readFrom(TiXmlElement * root,
                       std::vector<std::string> tag_sequence,
                       T & val)
  {
    if(tag_sequence.size() == 0)
    {
      std::stringstream ss(root->GetText());
      ss >> val;
      return;
    }
    readFrom(root->FirstChildElement(tag_sequence.front().c_str()),
    {tag_sequence.begin()+1, tag_sequence.end()},
             val);
  }

  std::tuple<bool, uint, std::string> parseLaser(const std::string &urdf_xml);

public:

  Robot(const std::string &robot_namespace, const Pose2D _pose,
        bool is_circle, double _radius,
        cv::Scalar _color, cv::Scalar _laser_color,
        double _linear_noise, double _angular_noise);

  std::pair<std::string, char> initFromURDF(bool force_scanner, bool zero_joints, bool static_tf);

  bool operator==(const Robot &other) const
  {
    return id == other.id;
  }
  bool operator!=(const Robot &other) const
  {
    return id != other.id;
  }

  bool isTwin(const std::pair<std::string, char> &other) const
  {
    return robot_namespace == other.first && id != other.second;
  }

  // grid access
  sensor_msgs::LaserScan scan;
  cv::Point2f pos_pix;
  cv::Scalar laser_color;
  cv::Scalar color;

  float radius;
  double x() const {return pose.x;}
  double y() const {return pose.y;}
  void display(cv::Mat &img) const;
  std::vector<cv::Point> contour() const
  {
    std::vector<cv::Point> poly;
    const double diagonal(radius / 1.414);
    for(auto corner: {0, 1, 2, 3})
    {
      float corner_angle = pose.theta + M_PI/4 + M_PI/2*corner;
      poly.emplace_back(pos_pix.x + diagonal*cos(corner_angle),
                        pos_pix.y + diagonal*sin(corner_angle));
    }
  return poly;
  }

  bool collidesWith(int u, int v) const;

  double x_l() const {return x() + laser_pose.x*cos(laser_pose.theta) - laser_pose.y*sin(laser_pose.theta);}
  double y_l() const {return y() + laser_pose.x*sin(laser_pose.theta) + laser_pose.y*cos(laser_pose.theta);}
  float theta_l() const {return pose.theta + laser_pose.theta;}

  // shared among robots
  static ros::NodeHandle* sim_node;

  void move(double dt);

  bool connected() const
  {
    return odom_pub.get();
  }

  bool hasLaser() const
  {
    return scan_pub.get();
  }

  void publish(const ros::Time &stamp, tf2_ros::TransformBroadcaster *br);
};

}

#endif // ROBOT_H
