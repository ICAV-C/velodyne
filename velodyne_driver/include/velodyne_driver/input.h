/* -*- mode: C++ -*-
 *
 *  Copyright (C) 2007 Austin Robot Technology, Yaxin Liu, Patrick Beeson
 *  Copyright (C) 2009, 2010 Austin Robot Technology, Jack O'Quin
 *
 *  License: Modified BSD Software License Agreement
 *
 *  $Id$
 */

/** \file
 *
 *  Velodyne HDL-64E 3D LIDAR data input classes
 *
 *  \ingroup velodyne
 *
 *    These classes provide raw Velodyne LIDAR input packets from
 *    either a live socket interface or a previously-saved PCAP dump
 *    file.
 *
 *  Classes:
 *
 *     velodyne::Input -- virtual base class than can be used to access
 *                      the data independently of its source
 *
 *     velodyne::InputSocket -- derived class reads live data from the
 *                      device via a UDP socket
 *
 *     velodyne::InputPCAP -- derived class provides a similar interface
 *                      from a PCAP dump
 */

#ifndef __VELODYNE_INPUT_H
#define __VELODYNE_INPUT_H

#include <unistd.h>
#include <stdio.h>
#include <pcap.h>

#include <ros/ros.h>
#include <velodyne_msgs/VelodynePacket.h>

namespace velodyne_driver
{
  static uint16_t UDP_PORT_NUMBER = 2368;

  // Pure virtual base Velodyne input class -- not used directly
  class Input
  {
  public:
    Input() {};

    /** \brief Read velodyne packet.
     *
     * \param pkt[out] points to VelodynePacket message
     *
     * \returns 0 if successful,
     *          -1 if end of file
     *          > 0 if incomplete packet (is this possible?)
     */
    virtual int getPacket(velodyne_msgs::VelodynePacket *pkt);

    /** \brief Read velodyne packets.
     *
     * \param buffer array to receive raw data packets
     * \param npacks number of packets to read
     * \param data_time[out] average time when data received
     *
     * \returns number of packets not read, if any
     *          0 if successful,
     *          -1 if end of file
     *
     * \deprecated Use getPacket() instead
     */
    virtual int getPackets(uint8_t *buffer, int npacks, double *data_time) = 0;

    /** \brief Close the data socket or file
     *
     * \returns 0, if successful
     *          errno value, for failure
     */
    virtual int vclose(void) = 0;

    /** \brief Open the data socket or file
     *
     * \returns: 0, if successful;
     *           -1 for failure
     * \todo return errno value for failure
     */
    virtual int vopen(void) = 0;
  };

  /** \brief Live Velodyne input from socket. */
  class InputSocket: public Input
  {
  public:
    InputSocket(uint16_t udp_port = UDP_PORT_NUMBER):
    Input()
    {
      udp_port_ = udp_port;
      sockfd_ = -1;
    }
    ~InputSocket() {}

    virtual int getPackets(uint8_t *buffer, int npacks, double *data_time);
    virtual int vclose(void);
    virtual int vopen(void);

  private:
    uint16_t udp_port_;
    int sockfd_;
  };


  /** \brief Velodyne input from PCAP dump file.
   *
   * Dump files can be grabbed by libpcap, Velodyne's DSR software,
   * ethereal, wireshark, tcpdump, or the \ref vdump_command.
   */
  class InputPCAP: public Input
  {
  public:
    InputPCAP(std::string filename="",
              bool read_once=false,
              bool read_fast=false,
              double repeat_delay=0.0):
      Input(), delay_(2600.0)
    {
      filename_ = filename;
      fp_ = NULL;  
      pcap_ = NULL;  
      empty_ = true;

      // get parameters from "input" subspace of private node handle
      ros::NodeHandle private_nh("~/input");
      private_nh.param("read_once", read_once_, read_once);
      private_nh.param("read_fast", read_fast_, read_fast);
      private_nh.param("repeat_delay", repeat_delay_, repeat_delay);

      if (read_once_)
        ROS_INFO("Read input file only once.");
      if (read_fast_)
        ROS_INFO("Read input file as quickly as possible.");
      if (repeat_delay_ > 0.0)
        ROS_INFO("Delay %.3f seconds before repeating input file.",
                 repeat_delay_);
    }
    ~InputPCAP() {}

    virtual int getPackets(uint8_t *buffer, int npacks, double *data_time);
    virtual int vclose(void);
    virtual int vopen(void);

  private:
    std::string filename_;
    FILE *fp_;
    pcap_t *pcap_;
    char errbuf_[PCAP_ERRBUF_SIZE];
    bool empty_;
    bool read_once_;
    bool read_fast_;
    double repeat_delay_;
    ros::Rate delay_;
  };

} // velodyne_driver namespace

#endif // __VELODYNE_INPUT_H