
#include <rtt_hw_interface/rtt_hw_interface.h>
#include <controller_manager/controller_manager.h>

//using namespace rtt_ros_kdl_tools;

RttHwInterface::RttHwInterface(RTT::TaskContext* owner):
owner_(owner)
{
    owner_->ports()->addPort("JointPosition", port_joint_position_in).doc("");
    owner_->ports()->addPort("JointVelocity", port_joint_velocity_in).doc("");
    owner_->ports()->addPort("JointTorque", port_joint_torque_in).doc("");

    owner_->ports()->addPort("JointPositionCommand", port_joint_position_cmd_out).doc("");
    owner_->ports()->addPort("JointVelocityCommand", port_joint_velocity_cmd_out).doc("");
    owner_->ports()->addPort("JointTorqueCommand", port_joint_torque_cmd_out).doc("");

    /*owner_->addProperty("robot_description",robot_description);
    owner_->addProperty("root_link",root_link);
    owner_->addProperty("tip_link",tip_link);*/
    std::string robot_description_param_name("robot_description");
    std::string root_link_param_name("root_link");
    std::string tip_link_param_name("tip_link");

    std::string robot_description, root_link, tip_link;

    if(!ros::param::get(robot_description_param_name, robot_description))
        ROS_ERROR( "Could not get %s param",robot_description_param_name.c_str());

    if(!ros::param::get(root_link_param_name, root_link))
        ROS_ERROR( "Could not get %s param",root_link_param_name.c_str());

    if(!ros::param::get(tip_link_param_name, tip_link))
        ROS_ERROR( "Could not get %s param",tip_link_param_name.c_str());

    urdf::Model urdf_model;
    sensor_msgs::JointState joint_state;

    if(!urdf_model.initString(robot_description)) {
        ROS_ERROR("Could not init URDF");
    }

    for (auto& joint : urdf_model.joints_)
    {
      if(joint.second->limits)
      {
        if(joint.second->limits->lower != joint.second->limits->upper)
        {
            ROS_INFO("Adding joint [%s]",joint.second->name.c_str());
            joint_state.name.push_back(joint.second->name);
        }
      }
    }


    //KDL::Chain chain;
    //KDL::Tree tree;


    //initChainFromROSParamURDF(tree,chain);
    //initJointStateFromKDLCHain(chain,joint_state);

    const std::vector<std::string>& joint_names = joint_state.name;
    //std::copy(joint_state.name.begin(),joint_state.name.end(),joint_names.begin());

    cmd_joint_velocity_.resize(joint_names.size());
    cmd_joint_position_.resize(joint_names.size());
    cmd_joint_effort_.resize(joint_names.size());

    state_joint_position_.resize(joint_names.size());
    state_joint_velocity_.resize(joint_names.size());
    state_joint_effort_.resize(joint_names.size());

    jnt_pos_in.setZero(joint_names.size());
    jnt_vel_in.setZero(joint_names.size());
    jnt_trq_in.setZero(joint_names.size());

    jnt_pos_cmd_out.setZero(joint_names.size());
    jnt_vel_cmd_out.setZero(joint_names.size());
    jnt_trq_cmd_out.setZero(joint_names.size());

    for(size_t i=0; i<joint_names.size(); ++i)
    {

      ROS_INFO("Registering joint [%s]",joint_names[i].c_str());
      hardware_interface::JointStateHandle state_handle(joint_names[i], &state_joint_position_[i], &state_joint_velocity_[i], &state_joint_effort_[i]);
      joint_state_interface_.registerHandle(state_handle);

      hardware_interface::JointHandle pos_handle(joint_state_interface_.getHandle(joint_names[i]), &cmd_joint_position_[i]);
      position_joint_interface_.registerHandle(pos_handle);

      hardware_interface::JointHandle vel_handle(joint_state_interface_.getHandle(joint_names[i]), &cmd_joint_velocity_[i]);
      velocity_joint_interface_.registerHandle(vel_handle);

      hardware_interface::JointHandle eff_handle(joint_state_interface_.getHandle(joint_names[i]), &cmd_joint_effort_[i]);
      effort_joint_interface_.registerHandle(eff_handle);

    }

    registerInterface(&joint_state_interface_);
    registerInterface(&velocity_joint_interface_);
    registerInterface(&position_joint_interface_);
    registerInterface(&effort_joint_interface_);
}
/*
void RttHwInterface::cleanup()
{

}
*/
void RttHwInterface::read()
{
    port_joint_position_in.readNewest(jnt_pos_in);
    port_joint_velocity_in.readNewest(jnt_vel_in);
    port_joint_torque_in.readNewest(jnt_trq_in);

    Eigen::Map<Eigen::VectorXd>(state_joint_position_.data(),state_joint_position_.size()) = jnt_pos_in;
    Eigen::Map<Eigen::VectorXd>(state_joint_velocity_.data(),state_joint_velocity_.size()) = jnt_vel_in;
    Eigen::Map<Eigen::VectorXd>(state_joint_effort_.data(),state_joint_effort_.size()) = jnt_trq_in;
}

void RttHwInterface::write()
{
    jnt_pos_cmd_out = Eigen::VectorXd::Map(cmd_joint_position_.data(),cmd_joint_position_.size());
    jnt_vel_cmd_out = Eigen::VectorXd::Map(cmd_joint_velocity_.data(),cmd_joint_velocity_.size());
    jnt_trq_cmd_out = Eigen::VectorXd::Map(cmd_joint_effort_.data(),cmd_joint_effort_.size());

    RTT::log(RTT::Debug) << "jnt_pos_cmd_out : "<<jnt_pos_cmd_out.transpose() << RTT::endlog();
    RTT::log(RTT::Debug) << "jnt_vel_cmd_out : "<<jnt_vel_cmd_out.transpose() << RTT::endlog();
    RTT::log(RTT::Debug) << "jnt_trq_cmd_out : "<<jnt_trq_cmd_out.transpose() << RTT::endlog();
    port_joint_position_cmd_out.write(jnt_pos_cmd_out);
    port_joint_velocity_cmd_out.write(jnt_vel_cmd_out);
    port_joint_torque_cmd_out.write(jnt_trq_cmd_out);
}
