// Copyright 2015 ISIR-CNRS
// Author: Antoine Hoarau

#ifndef __RTT_LWR_ABSTRACT_HPP__
#define __RTT_LWR_ABSTRACT_HPP__

#include <rtt/RTT.hpp>
#include <rtt/Attribute.hpp>
#include <rtt/Component.hpp>
#include <rtt_rosparam/rosparam.h>
#include <rtt_rosclock/rtt_rosclock.h>
#include <rtt/os/TimeService.hpp>
#include <rtt/Logger.hpp>
#include <rtt_roscomm/rtt_rostopic.h>

#include <kdl/frames.hpp>
#include <kdl/jntarray.hpp>
#include <kdl/jacobian.hpp>
#include <kuka_lwr_fri/friComm.h>

#include <lwr_fri/CartesianImpedance.h>
#include <lwr_fri/FriJointImpedance.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Wrench.h>
#include <geometry_msgs/Twist.h>

#include <tf_conversions/tf_kdl.h>

#include <Eigen/Dense>

#include <vector>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <boost/circular_buffer.hpp>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <rtt_ros_kdl_tools/tools.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <eigen_conversions/eigen_kdl.h>
#include <eigen_conversions/eigen_msg.h>
#include <kdl_conversions/kdl_msg.h>
#include <tf_conversions/tf_eigen.h>
namespace lwr{
class SegmentIndice{
    public: const int operator()(const std::string& segment_name)
    {
        return this->operator[](segment_name);
    }
    public: const int operator[](const std::string& segment_name)
    {
        if(seg_idx_names.empty())
            return -1;
        if(seg_idx_names[segment_name])
            return seg_idx_names[segment_name];
        else{
            std::map<std::string,int>::iterator it;
            it = seg_idx_names.end();
            it--;
            std::cerr << "Segment name ["<<segment_name<<"] does not exists, returning last element in map ["<<it->second<<"]" << std::endl;
            return it->second;
        }
    }
    public: void add(const std::string& seg_name,int i)
    {
        seg_idx_names[seg_name] = i;
    }
    protected : std::map<std::string,int> seg_idx_names;
};
    // Custom FRI/KRL Cmds
    static const fri_int32_t FRI_START = 1;
    static const fri_int32_t FRI_STOP = 2;
    static const fri_int32_t STOP_KRL_SCRIPT = 3;
    
class RTTLWRAbstract : public RTT::TaskContext{
  public:
      RTTLWRAbstract(std::string const& name);
      virtual ~RTTLWRAbstract();
     /** @brief Orocos Configure Hook
     * Initialization of the shared array between
     * KRC and remote pc
     * We choose by convention to trigger fri_start() in
     * the KRL program if $FRI_FRM_INT[1] == 1
     */
    virtual bool configureHook(){this->init(true);}
    /** @brief Orocos Start Hook
     * Send arrays to KRC and call doStart()
     */
    bool startHook();

    /** @brief To implement if specific things have to be done when starting the component
     */
    virtual bool doStart();

    /** @brief Orocos Update hook
     */
    //virtual void updateHook() = 0;

    /** @brief Orocos stop hook
     * Call doStop(), then put 2 in $FRI_FRM_INT[1]
     * which by our convention trigger fri_stop() in KRL program
     */
    virtual void stopHook();

    /** @brief To implement if specific things have to be done when stoping the component
     */
    virtual void doStop();

    /** @brief Orocos Cleanup hook
     */
    virtual void cleanupHook();
    
    const unsigned int getNrOfJoints() const;

    bool sendJointCommand(RTT::OutputPort<Eigen::VectorXd>& port_cmd,const Eigen::VectorXd& jnt_cmd); 

    int getToolKRL();

    /** @brief Set control strategy
     */
    void setControlStrategy(const unsigned int mode);

    /** @brief Select the tool defined on the KRC
     */
    void setToolKRL(const unsigned int toolNumber);

    /** @brief Get the FRI Mode
     *  @return : The value of the FRI_STATE enum
     */
    FRI_STATE getFRIMode();
    
    /** @brief Get the FRI Quality
     *  @return : The value of the FRI_QUALITY enum
     */
    FRI_QUALITY getFRIQuality();
    
    /** @brief Get the FRI Control mode
     *  @return: The current control mode from FRI_CTRL (joint impedance, cartesian impedance or joint position)
     */
    FRI_CTRL getFRIControlMode();
    
    /** @brief Ask KRL script for a friStop()
     */
    void friStop();

    /** @brief Ask KRL script for a friStart()
     */
    void friStart();

    /** @brief Reset the array shared with the KRC
     */
    void friReset();

    /** @brief Ask KRL exit
     */
    void stopKrlScript();

    /** @brief Return the cartesian position of the tool center point in the robot base frame
     */
    bool getCartesianPosition(geometry_msgs::Pose& cart_position);

    /** @brief Return the Jacobian
     */
    bool getJacobian(KDL::Jacobian& jacobian);

    /** @brief Return the Mass Matrix
     */
    bool getMassMatrix(Eigen::MatrixXd& mass_matrix);

    /** @brief Return the gravity torque
     */
    bool getGravityTorque(Eigen::VectorXd& gravity_torque);
    /** @brief Return the estimated joint velocity
     */
    bool getJointVelocity(Eigen::VectorXd& joint_velocity);

    /** @brief Return the current configuration of the robot
     */
    bool getJointPosition(Eigen::VectorXd& joint_position);

    /** @brief Return the estimated external joint torque
     */
    bool getJointTorque(Eigen::VectorXd& joint_torque);
        /** @brief Return the actuator joint torque seem by the motor
     */
    bool getJointTorqueRaw(Eigen::VectorXd& joint_torque_raw);
    
    /** @brief Return the cartesian velocity of the Tooltip (Twist)
     */
    bool getCartesianVelocity(geometry_msgs::Twist& cart_twist);
    
    /** @brief Return the estimated external tool center point wrench
     */
    bool getCartesianWrench(geometry_msgs::Wrench& cart_wrench);

    /** @brief Send Joint position in radians
     */
    void sendJointPosition(const Eigen::VectorXd& joint_position_cmd);
    /** @brief Send Joint Impedance gains
     */
    void sendJointImpedance(const lwr_fri::FriJointImpedance& joint_impedance_cmd);
    /** @brief Send Joint Torque in N.m
     */
    void sendJointTorque(const Eigen::VectorXd& joint_torque_cmd);
    /** @brief Send Cartesian position Command
     */
    void sendCartesianPosition(const geometry_msgs::Pose& cartesian_pose);
    /** @brief Send Cartesian Wrench Command
     */
    void sendCartesianWrench(const geometry_msgs::Wrench& cartesian_wrench);
    /** @brief Set the Position Control Mode 10
     */
    void setJointPositionControlMode();
        
    /** @brief Set the Impedance Control Mode 30
     */
    void setJointImpedanceControlMode();
        
    /** @brief Set the Cartesian Impedance Control Mode 20
     */
    void setCartesianImpedanceControlMode();
    bool isCommandMode();
    bool isMonitorMode();
    bool isPowerOn();
    bool isJointImpedanceControlMode();
    bool isCartesianImpedanceControlMode();
    bool isJointPositionControlMode();
    bool updateState();
    
    template<class T>
    RTT::FlowStatus readData(RTT::InputPort<T>& port,T& data)
    {
        if(port.connected() == false)
        {
            RTT::log(RTT::Error) << port.getName() << " not connected "<< RTT::endlog();
            return RTT::NoData;
        }
        return port.read(data);
    }
    
    template<class T>
    void writeData(RTT::InputPort<T>& port,const T& data)
    {
        if(port.connected() == false)
        {
            RTT::log(RTT::Error) << port.getName() << " not connected "<< RTT::endlog();
            return false;
        }
        port.write(data);
    }
    
    bool getAllComponentRelative();
    
    void setJointTorqueControlMode();

    bool connectAllPorts(const std::string& robot_name="lwr");
private:
    /**
     * @brief lwr or lwr_sim peer (used to connect ports)
     */
      RTT::TaskContext* peer;
protected:
    static const unsigned int n_joints = LBR_MNJ;

    bool init(bool connect_all_ports=true);
    /**
     * @brief Shared arrays from the remote pc to the KRC
     */
    tFriKrlData fri_to_krl;

    /**
     * @brief Shared arrays from the KRC to the remote pc
     */
    tFriKrlData fri_from_krl;

    RTT::OutputPort<lwr_fri::CartesianImpedance > port_CartesianImpedanceCommand;
    RTT::OutputPort<geometry_msgs::Wrench > port_CartesianWrenchCommand;
    RTT::OutputPort<geometry_msgs::Pose > port_CartesianPositionCommand;
    RTT::OutputPort<lwr_fri::FriJointImpedance > port_JointImpedanceCommand;
    RTT::OutputPort<Eigen::VectorXd > port_JointPositionCommand;
    RTT::OutputPort<Eigen::VectorXd > port_JointTorqueCommand;
    //RTT::OutputPort<std_msgs::Int32 > port_KRL_CMD;
    RTT::InputPort<tFriKrlData > port_FromKRL;
    //RTT::InputPort<std_msgs::Int32 > port_KRL_CMD;
        
    RTT::OutputPort<tFriKrlData > port_ToKRL;
    RTT::InputPort<geometry_msgs::Wrench > port_CartesianWrench;
    RTT::InputPort<tFriRobotState > port_RobotState;
    RTT::InputPort<tFriIntfState > port_FRIState;
    RTT::InputPort<Eigen::VectorXd > port_JointVelocity;
    RTT::InputPort<geometry_msgs::Twist > port_CartesianVelocity;
    RTT::InputPort<geometry_msgs::Pose > port_CartesianPosition;
    RTT::InputPort<Eigen::MatrixXd > port_MassMatrix;
    RTT::InputPort<KDL::Jacobian > port_Jacobian;
    RTT::InputPort<Eigen::VectorXd> port_JointTorque;
    RTT::InputPort<Eigen::VectorXd> port_GravityTorque;
    RTT::InputPort<Eigen::VectorXd> port_JointPosition;
    RTT::InputPort<Eigen::VectorXd> port_JointTorqueRaw;
    RTT::InputPort<Eigen::VectorXd> port_JointPositionFRIOffset;

    tFriRobotState robot_state;
    tFriIntfState fri_state;
    geometry_msgs::Pose cart_pos, cart_pos_cmd;
    geometry_msgs::Wrench cart_wrench, cart_wrench_cmd;
    geometry_msgs::Twist cart_twist;
    KDL::Twist cart_twist_kdl;
    Eigen::MatrixXd mass;
    lwr_fri::FriJointImpedance jnt_imp_cmd;
    lwr_fri::CartesianImpedance cart_imp_cmd;
    
    Eigen::Matrix<double,6,1> X,Xd,Xdd,X_cmd,Xd_cmd,Xdd_cmd;
    Eigen::Affine3d X_aff,Xd_aff;
    Eigen::VectorXd kp,kd;
    Eigen::Matrix<double,6,1> kp_cart,kd_cart;
    KDL::Frame X_kdl,Xd_kdl;
    
    Eigen::VectorXd jnt_pos,
                    jnt_pos_old,
                    jnt_trq,
                    jnt_trq_raw,
                    jnt_pos_fri_offset,
                    jnt_grav,
                    jnt_vel,
                    jnt_pos_cmd,
                    jnt_trq_cmd;

    KDL::JntArray   jnt_pos_kdl,
                    jnt_vel_kdl,
                    coriolis_kdl,
                    gravity_kdl,
                    jnt_trq_kdl,
                    jnt_acc_kdl;
                    
    KDL::JntArrayVel jnt_pos_vel_kdl;
    KDL::Frame tool_in_base_frame;

    KDL::Wrenches f_ext_kdl;

    KDL::Jacobian J_tip_base;
    KDL::FrameVel tool_in_base_framevel;
    
    std::string root_link,tip_link,robot_name,robot_description;
    urdf::Model urdf_model;
    KDL::Tree kdl_tree;
    KDL::Chain kdl_chain;        
        
    boost::scoped_ptr<KDL::ChainFkSolverVel_recursive> fk_vel_solver;
    boost::scoped_ptr<KDL::ChainDynParam> id_dyn_solver;
    boost::scoped_ptr<KDL::ChainJntToJacSolver> jnt_to_jac_solver;
    boost::scoped_ptr<KDL::ChainIdSolver_RNE> id_rne_solver;

    KDL::Vector gravity_vector;
    SegmentIndice seg_names_idx;

    bool connect_all_ports_at_startup,use_sim_clock;
};
}
#endif
