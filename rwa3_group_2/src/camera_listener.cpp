#include "camera_listener.h"


   void CameraListener::logical_camera_callback(
       const nist_gear::LogicalCameraImage::ConstPtr &msg, int cam_idx)
  // void CameraListener::logical_camera_callback(
  //     const nist_gear::LogicalCameraImage::ConstPtr &msg)
  {

    std::unordered_map<std::string, int> part_count;
    camera_parts_list_[cam_idx].clear();
    if(msg->models.size()) {
    ROS_INFO_STREAM("================== Logical camera " << cam_idx << " ==================");
    ROS_INFO_STREAM("Logical camera: '" << msg->models.size());

      /* Start: Get TF of camera */
      geometry_msgs::TransformStamped transformStamped;
      if (msg->models.size()) {

        ros::Duration timeout(1.0);
        bool transform_detected = false;
        std::string cam_id = "logical_camera_" + std::to_string(cam_idx) + "_frame";

        while (!transform_detected) {
          try{
            transformStamped = tfBuffer.lookupTransform("world", cam_id,
                                     ros::Time(0), timeout);
            transform_detected = true;
          }
          catch (tf2::TransformException &ex) {
            ROS_WARN("%s",ex.what());
            ros::Duration(1.0).sleep();
            continue;
          }
        }
      }
    //ROS_INFO_STREAM("transformStamped = "<< transformStamped);
    /* End: Get TF of camera */

    for (int i = 0; i < msg->models.size(); i++) {

      // ROS_INFO_STREAM("Model = " << msg->models[i].type << "\nPose = "
      //     << msg->models[i].pose << "\n");

      ModelInfo temp_model;
      temp_model.cam_index = cam_idx;
      temp_model.camera_pose = msg->pose;
      temp_model.model_pose = msg->models[i].pose;
      temp_model.transformStamped = transformStamped;

      std::string color = msg->models[i].type;
      std::string delimiter = "_";

      temp_model.type = color.substr(0, color.find(delimiter));

      if(part_count.find(msg->models[i].type) == part_count.end()) {
        part_count[msg->models[i].type] = 1;
      } else {
        part_count[msg->models[i].type]++;
      }
      int frame_number_to_append{part_count[msg->models[i].type]};

      int pos{};
    
      while ((pos = color.find(delimiter)) != std::string::npos) {
         color.erase(0,pos+delimiter.length());
      }
      temp_model.color = color;

    //perform TF
    performTransform(&temp_model);

      temp_model.id = "logical_camera_" + std::to_string(cam_idx) + "_" + msg->models[i].type + "_" + std::to_string(frame_number_to_append) + "_frame";
      camera_parts_list_[cam_idx].push_back(temp_model); // add copy of our struct to array of vectors 
      // ROS_INFO_STREAM(temp_model.id);
    }
  }
  }

  // Function to perform transform without passing actual frame ids
  void CameraListener::performTransform(ModelInfo *model) {

    geometry_msgs::PoseStamped pose_target, pose_rel;
    std::string cam_id = "logical_camera_" + std::to_string(model->cam_index) + "_frame";
    pose_rel.header.frame_id = cam_id;
    pose_rel.pose = model->model_pose;

    tf2::doTransform(pose_rel, pose_target, model->transformStamped);
    (*model).world_pose.position.x = pose_target.pose.position.x;
    (*model).world_pose.position.y = pose_target.pose.position.y;
    (*model).world_pose.position.z = pose_target.pose.position.z;
    (*model).world_pose.orientation = pose_target.pose.orientation;

  }

  void CameraListener::sort_camera_parts_list() {

    //std::unordered_map<std::string, std::unordered_map<std::string, std::vector<ModelInfo>>> ordered_color_type;
    ordered_color_type.clear();

    for(auto row: camera_parts_list_) {
      for(auto model: row) {
        ordered_color_type[model.color][model.type].push_back(model);
      }
    }

    std::array<std::string, 3> colors{"red", "blue", "green"};
    std::array<std::string, 4> types{"disk", "pulley", "gasket", "piston"};

    for(auto color: colors) {
      for(auto type: types) {
        ROS_INFO_STREAM("\n====================== " << color << " " << type << " ======================");
        for (auto model : ordered_color_type[color][type]) {
          ROS_INFO_STREAM(model.id);

          tf2::Quaternion q(
             model.world_pose.orientation.x,
             model.world_pose.orientation.y,
             model.world_pose.orientation.z,
             model.world_pose.orientation.w);
          tf2::Matrix3x3 m(q);
          double roll, pitch, yaw;
          m.getRPY(roll, pitch, yaw);
          ROS_INFO("Position(XYZ): [%f,%f,%f] || Orientation(RPY): [%f,%f,%f]",model.world_pose.position.x,
          model.world_pose.position.y,
          model.world_pose.position.z,
          roll,
          pitch,
          yaw);
          
        }
      }
    }
  }

CameraListener::CameraListener(ros::NodeHandle & node)
    : tfBuffer(), tfListener(tfBuffer) {
      node_ = node;
    }


//int main(int argc, char **argv)
std::array<std::vector<CameraListener::ModelInfo>,16> CameraListener::fetchParts(ros::NodeHandle &node)
{

  const int num_cams = 16; // ADD NUM CAMS
  ROS_INFO("Subscribing for camera");

  std::array<ros::Subscriber, num_cams> logical_camera_subscriber{};
    for(int i = 0; i < num_cams; i++) {
        logical_camera_subscriber[i] = node.subscribe<nist_gear::LogicalCameraImage> (
          "/ariac/logical_camera_"+std::to_string(i), 10,
          boost::bind(&CameraListener::logical_camera_callback, this, _1, i));
    }

       ROS_INFO("Param is set. Query the logical cameras");
       ros::Duration(0.2).sleep();
       CameraListener::sort_camera_parts_list();

return camera_parts_list_;

}