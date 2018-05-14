/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c)  2012 The Provost, Fellows and Scholars of the 
 * College of the Holy and Undivided Trinity of Queen Elizabeth near Dublin. 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Xiuchao Wu <xw2@cs.ucc.ie>
 */

/**************************************************************
*
* This script will create an UDP echo client and server application
* running in the OLT and the ONU respectively. These ONUs and the OLT will be in
* the same subnet
*                                                                            P2P
*                                                       ------------ ONU1 ---------- UDP sink/sender 1
*                       P2P              P2P (CN)      /
*  UDP sender/sink 1 ---------- Gateway ------------ OLT
*                                |                     \                     P2P
*                        P2P     |                      ------------ ONU2 ---------- UDP sink/sender 2
*   UDP sender/sink 2 ------------
**************************************************************/


#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/object-factory.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/xgpon-module.h"

#include "ns3/netanim-module.h"

//Stats module for tracing
#include "ns3/stats-module.h"





#define APP_START_TIME   0
#define APP_STOP_TIME   10
#define SIM_STOP_TIME   (APP_STOP_TIME + 1)

#define UDP_PKT_SIZE     1500
#define DS_PKT_INTERVAL  0.01
#define US_PKT_INTERVAL  0.04


//256 ONUs with UDP traffics in both directions (10Mbps per onu per direction).
//more than 1000 ONUs (at most 1023 onus since onuid is in the reange of 0...1022 and 1023 is used for broadcast ploam messages) 
//and 10Gbps traffic load (10Mbps per onu) do work although the simulation has no progress for a long time.
//The main reason is Ipv4GlobalRoutingHelper::PopulateRoutingTables(). It takes a very long time (one day) to setup the routing tables for all nodes.
#define ONU_NUM          2  


#define ONU_TX_TRACE_FILE_BASE "data/xgpon/OnuTraceTx"
#define ONU_RX_TRACE_FILE_BASE "data/xgpon/OnuTraceRx"
#define ONU_TRACE_FILE "data/xgpon/OnuTraceTest.txt"
#define OLT_TX_TRACE_FILE "data/xgpon/OltTxTraceTest.txt"
#define OLT_RX_TRACE_FILE "data/xgpon/OltRxTraceTest.txt"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("XgponAnimationUdp");

void
TxTrace (std::string path, Ptr<const XgponDsFrame> pkt, Time t)
{
  std::ofstream myfile;
  myfile.open (path.c_str(), std::ios::app);
  myfile<<"Tx Time: "<<t<<"\n";
  pkt->Print(myfile);
  myfile<<"\n\n";
  myfile.close();
}

void
RxTrace (std::string path, Ptr<const Packet> pkt, Time t)
{
  std::ofstream myfile;
  myfile.open (path.c_str(), std::ios::app);
  myfile<<"Rx Time: "<<t<<"\n";
  pkt->Print(myfile);
  myfile<<"\n\n";
  myfile.close();
}


int 
main (int argc, char *argv[])
{
  bool p_verbose = false;
  uint16_t p_nOnus = ONU_NUM;  
  uint16_t p_appStartTime = APP_START_TIME;
  uint16_t p_appStopTime = APP_STOP_TIME;
  uint16_t p_simStopTime = SIM_STOP_TIME;
  uint16_t p_pktSize = UDP_PKT_SIZE;
  double p_dsPktInterval = DS_PKT_INTERVAL;
  double p_usPktInterval = US_PKT_INTERVAL;
  std::string p_animFilename = "udp-animation.xml";
  


  CommandLine cmd;
  cmd.AddValue ("verbose", "Tell application to log if true", p_verbose);
  cmd.AddValue ("onus", "the number of onus", p_nOnus);
  cmd.AddValue ("astarttime", "the start time of applications", p_appStartTime);
  cmd.AddValue ("astoptime", "the stop time of applications", p_appStopTime);
  cmd.AddValue ("sstoptime", "the stop time of whole simulation", p_simStopTime);
  cmd.AddValue ("dspktinterval", "the packet interval of downstream udp client (second)", p_dsPktInterval);
  cmd.AddValue ("uspktinterval", "the packet interval of upstream udp client (second)", p_usPktInterval);
  cmd.AddValue ("pktsize", "the UDP packet size (byte)", p_pktSize);
  cmd.AddValue ("animFile", "the name of the animation trace file", p_animFilename);



  cmd.Parse (argc,argv);

  if(p_verbose)
  {
    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  }
  //LogComponentEnable ("XgponChannel", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponOltNetDevice", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponOnuNetDevice", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponXgemEngine", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponOltXgemEngine", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponOltFramingEngine", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponPhy", LOG_LEVEL_FUNCTION);
  //LogComponentEnable ("XgponOnuXgemEngine", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOnuDbaEngine", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOnuFramingEngine", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOnuPhy", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOltSimpleDsScheduler", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOltDbaEngineRoundRobin", LOG_LEVEL_LOGIC);
  //LogComponentEnable ("XgponOltDsSchedulerRoundRobin", LOG_LEVEL_LOGIC);

  Packet::EnablePrinting ();

  std::cout << "number of ONUs: " << p_nOnus << std::endl;



  //////////////////////////////////////////Create all nodes and organize into the corresponding containers for installing network devices.
  ///////the ONUs, OLT nodes, and container for all xgpon nodes
  NodeContainer oltNode, onuNodes, xgponNodes;
  oltNode.Create (1);
  onuNodes.Create (p_nOnus);
  xgponNodes.Add(oltNode.Get(0));
  for(int i=0; i<p_nOnus; i++) { xgponNodes.Add (onuNodes.Get(i)); }


  ///////the gateway node, OLT node, and container for the link to simulate Internet's core network
  NodeContainer gatewayNode, cnNodes;
  gatewayNode.Create (1);
  cnNodes.Add(oltNode.Get(0));
  cnNodes.Add(gatewayNode.Get(0));


  ///////the end hosts at both sides of the networks  
  NodeContainer leftNodes, rightNodes;
  NodeContainer leftLinkNodes[p_nOnus];
  NodeContainer rightLinkNodes[p_nOnus];
  leftNodes.Create (p_nOnus);
  rightNodes.Create (p_nOnus);
  for(int i=0; i<p_nOnus; i++) 
  { 
    leftLinkNodes[i].Add(leftNodes.Get(i));
    leftLinkNodes[i].Add(gatewayNode.Get(0));

    rightLinkNodes[i].Add(rightNodes.Get(i));
    rightLinkNodes[i].Add(onuNodes.Get(i));
  }









  /////////////////////////////////////////Create all links used to connect the above nodes
  ///////Xgpon network configuration through XgponHelper
  XgponHelper xgponHelper;
  XgponConfigDb& xgponConfigDb = xgponHelper.GetConfigDb ( );

  xgponConfigDb.SetOltNetmaskLen (16);
  xgponConfigDb.SetOnuNetmaskLen (24);
  xgponConfigDb.SetIpAddressFirstByteForXgpon (10);
  xgponConfigDb.SetIpAddressFirstByteForOnus (172);


  //other configuration related information 
  Config::SetDefault("ns3::XgponOltDbaEngineRoundRobin::MaxServiceSize", UintegerValue(1000));  //1000 words
  Config::SetDefault("ns3::XgponOltDsSchedulerRoundRobin::MaxServiceSize", UintegerValue(10000));  //10K bytes
  Config::SetDefault("ns3::XgponOnuUsSchedulerRoundRobin::MaxServiceSize", UintegerValue(4000));  //8K bytes


  //Set TypeId String for object factories through XgponConfigDb before the following call.
  //initialize object factories
  xgponHelper.InitializeObjectFactories ( );

  //configuration through object factory
  xgponHelper.SetQueueAttribute ("MaxPackets", UintegerValue(50));  //queue size is 50KBytes
  xgponHelper.SetQueueAttribute ("MaxBytes", UintegerValue(50000));  //queue size is 50KBytes

  //install xgpon network devices
  NetDeviceContainer xgponDevices = xgponHelper.Install (xgponNodes);





  ///////Internet core network through PointToPointHelper
  PointToPointHelper p2pHelper;

  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer cnDevices = p2pHelper.Install (cnNodes);


  ///////access links for end hosts through PointToPointHelper
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("10000Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer leftLinkDevices[p_nOnus];
  NetDeviceContainer rightLinkDevices[p_nOnus];
  for(int i=0; i<p_nOnus; i++) 
  { 
    leftLinkDevices[i] = p2pHelper.Install (leftLinkNodes[i]);
    rightLinkDevices[i] = p2pHelper.Install (rightLinkNodes[i]);
  }






  /////////////////////////////////////////////////////////////////////////////////install internet protocol stack
  InternetStackHelper stack;
  stack.Install (xgponNodes);
  stack.Install (leftNodes);
  stack.Install (rightNodes);
  stack.Install (gatewayNode);






  ///////////////////////////////////////////////////////////////////////////////Assign IP addresses to all interfaces of all nodes
  Ipv4AddressHelper addressHelper;

  ///////////Assign IP addresses to core network nodes (point-to-point link)
  std::string cnIpbase = xgponHelper.GetIpAddressBase (150, 0, 24);
  std::string cnNetmask = xgponHelper.GetIpAddressNetmask (24); 
  addressHelper.SetBase (cnIpbase.c_str(), cnNetmask.c_str());
  Ipv4InterfaceContainer cnInterfaces = addressHelper.Assign (cnDevices);
  if(p_verbose)
  {
    Ipv4Address tmpAddr = cnInterfaces.GetAddress(0);
    std::cout << "OLT Internet Interface's IP Address: ";
    tmpAddr.Print(std::cout);
    std::cout << std::endl;

    tmpAddr = cnInterfaces.GetAddress(1);
    std::cout << "Internet Gateway's IP Address: ";
    tmpAddr.Print(std::cout);
    std::cout << std::endl;
  }

  
  /////////Assign IP addresses to end hosts at the left side (point-to-point link)
  Ipv4InterfaceContainer leftLinkInterfaces[p_nOnus];
  for(int i=0; i<p_nOnus; i++) 
  {
    std::string leftIpbase = xgponHelper.GetIpAddressBase (160, i, 24);
    std::string leftNetmask = xgponHelper.GetIpAddressNetmask (24); 
    addressHelper.SetBase (leftIpbase.c_str(), leftNetmask.c_str());

    leftLinkInterfaces[i] = addressHelper.Assign (leftLinkDevices[i]);
    if(p_verbose)
    {
      Ipv4Address addr = leftLinkInterfaces[i].GetAddress(0);
      std::cout << "Left node " << i <<" IP Address: ";
      addr.Print(std::cout);
      std::cout << std::endl;

      addr = leftLinkInterfaces[i].GetAddress(1);
      std::cout << "Corresponding IP Address at the gateway node: ";
      addr.Print(std::cout);
      std::cout << std::endl;
    }
  }





  ///////////Assign IP addresses to OLT and ONU (for xgpon network devices)
  Ptr<XgponOltNetDevice> tmpDevice = DynamicCast<XgponOltNetDevice, NetDevice> (xgponDevices.Get(0));
  std::string xgponIpbase = xgponHelper.GetXgponIpAddressBase ( );
  std::string xgponNetmask = xgponHelper.GetOltAddressNetmask();
  addressHelper.SetBase (xgponIpbase.c_str(), xgponNetmask.c_str());

  Ipv4InterfaceContainer xgponInterfaces = addressHelper.Assign (xgponDevices);
  for(int i=0; i<(p_nOnus+1);i++)
  {
    Ipv4Address addr = xgponInterfaces.GetAddress(i);
    Ptr<XgponNetDevice> tmpDevice = DynamicCast<XgponNetDevice, NetDevice> (xgponDevices.Get(i));
    tmpDevice->SetAddress (addr);

    if(p_verbose)
    {
      if(i==0) std::cout << "OLT IP Address: ";
      else std::cout << "ONU " << (i-1) <<" IP Address: ";
      addr.Print(std::cout);
      std::cout << std::endl;
    }
  }



  /////////Assign IP addresses to end hosts at the right side (point-to-point link)
  Ipv4InterfaceContainer rightLinkInterfaces[p_nOnus];

  for(int i=0; i<p_nOnus; i++) 
  {
    Ptr<XgponOnuNetDevice> tmpDevice = DynamicCast<XgponOnuNetDevice, NetDevice> (xgponDevices.Get(i+1));
    std::string onuIpbase = xgponHelper.GetOnuIpAddressBase (tmpDevice);
    std::string onuNetmask = xgponHelper.GetOnuAddressNetmask();     
    addressHelper.SetBase (onuIpbase.c_str(), onuNetmask.c_str());

    rightLinkInterfaces[i] = addressHelper.Assign (rightLinkDevices[i]);
    if(p_verbose)
    {
      Ipv4Address addr = rightLinkInterfaces[i].GetAddress(0);
      std::cout << "Right Node " << i <<" IP Address: ";
      addr.Print(std::cout);
      std::cout << std::endl;

      addr = rightLinkInterfaces[i].GetAddress(1);
      std::cout << "IP Address at the Corresponding ONU: ";
      addr.Print(std::cout);
      std::cout << std::endl;
    }
  }




  //////////////////////////////////////////////////////////////////////////////////////Add OMCI Channels
  //set attributes (QoS parameters, etc.) for connections to be added as OMCI channel
  //we need get the address before setting OMCI channel.
  //for(int i=0; i<p_nOnus; i++) 
  //{ 
  //  Ptr<XgponOltNetDevice> oltDevice = DynamicCast<XgponOltNetDevice, NetDevice> (xgponDevices.Get(0));
  //  Ptr<XgponOnuNetDevice> onuDevice = DynamicCast<XgponOnuNetDevice, NetDevice> (xgponDevices.Get(i+1));
  //  xgponHelper.AddOmciConnectionsForOnu (onuDevice, oltDevice); 
  //}




  /////////////////////////////////////////////////////////////////////add xgem ports for end hosts connected to ONUs
  for(int i=0; i<p_nOnus; i++) 
  {
    Address addr = rightLinkInterfaces[i].GetAddress(0);

    Ptr<XgponOltNetDevice> oltDevice = DynamicCast<XgponOltNetDevice, NetDevice> (xgponDevices.Get(0));
    Ptr<XgponOnuNetDevice> onuDevice = DynamicCast<XgponOnuNetDevice, NetDevice> (xgponDevices.Get(i+1));

    uint16_t allocId = xgponHelper.AddOneTcontForOnu (onuDevice, oltDevice);
    uint16_t upPortId = xgponHelper.AddOneUpstreamConnectionForOnu (onuDevice, oltDevice, allocId, addr);
    uint16_t downPortId = xgponHelper.AddOneDownstreamConnectionForOnu (onuDevice, oltDevice, addr); 

    if(p_verbose) 
    {
      std::cout << "ONU-ID = "<<onuDevice->GetOnuId() << ";   ALLOC-ID = " << allocId << ";   UP-PORT-ID = " << upPortId << ";   DOWN-PORT-ID = " << downPortId << std::endl;
    }    
  }




  //////////////////////////////////////////////////////////Populate routing tables for all nodes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();






  //////////////////////////////////////////////////////////Configure traffics
  ///////////Containers (downstream)
  ApplicationContainer leftServers, rightServers;
  uint16_t leftServerPort=9000;
  uint16_t rightServerPort=9001;

  //Set UdpServer on left nodes
  UdpServerHelper leftServerHelper (leftServerPort);
  leftServers = leftServerHelper.Install (leftNodes);
  leftServers.Start (Seconds (0));
  leftServers.Stop (Seconds (p_appStopTime));

  //Set UdpServer on right nodes
  UdpServerHelper rightServerHelper (rightServerPort);
  rightServers = rightServerHelper.Install (rightNodes);
  rightServers.Start (Seconds (0));
  rightServers.Stop (Seconds (p_appStopTime));




  //SetUdpClient on left nodes (generate downstream traffic); connect to right servers
  for(int i=0; i<p_nOnus; i++)
  {
    UdpClientHelper udpClientHelper (rightLinkInterfaces[i].GetAddress(0), rightServerPort);
    udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (2000000000));
    udpClientHelper.SetAttribute ("Interval", TimeValue (Seconds (p_dsPktInterval)));
    udpClientHelper.SetAttribute ("PacketSize", UintegerValue (p_pktSize));

    ApplicationContainer clientApp = udpClientHelper.Install (leftNodes.Get (i));
    clientApp.Start (Seconds (p_appStartTime + i * 0.01));
    clientApp.Stop (Seconds (p_appStopTime));
  }


  //SetUdpClient on right nodes (generate upstream traffic); connect to left servers
  for(int i=0; i<p_nOnus; i++)
  {
    UdpClientHelper udpClientHelper (leftLinkInterfaces[i].GetAddress(0), leftServerPort);
    udpClientHelper.SetAttribute ("MaxPackets", UintegerValue (2000000000));
    udpClientHelper.SetAttribute ("Interval", TimeValue (Seconds (p_usPktInterval)));
    udpClientHelper.SetAttribute ("PacketSize", UintegerValue (p_pktSize));

    ApplicationContainer clientApp = udpClientHelper.Install (rightNodes.Get (i));
    clientApp.Start (Seconds (p_appStartTime + i * 0.01));
    clientApp.Stop (Seconds (p_appStopTime));
  }






  if(p_verbose)
  {
    xgponHelper.EnableAsciiAll("xgpon-simulation-speed-udp-ascii");
    xgponHelper.EnablePcapAll("xgpon-simulation-speed-udp-pcap");
  }


  //Ptr<XgponOltNetDevice> oltDevice = DynamicCast<XgponOltNetDevice, NetDevice> (xgponDevices.Get(0));
  // Tracing stuff
  //devOnus[0]->TraceConnect ("PhyTxEnd", "test1", MakeCallback(&TxTrace));
  //devOnus[0]->TraceConnect ("PhyRxEnd", "test2", MakeCallback(&RxTrace));
  //oltDevice->TraceConnect ("PhyTxEnd", OLT_TX_TRACE_FILE, MakeCallback(&TxTrace));
  //devOlt->TraceConnect ("PhyRxEnd", "test4", MakeCallback(&RxTrace));

  // Create the animation object and configure for specified output
  AnimationInterface::SetConstantPosition (gatewayNode.Get (0), 100, 10); 
  AnimationInterface::SetConstantPosition (oltNode.Get (0), 150, 10); 

  for(int i=0; i<p_nOnus; i++)
  {
    AnimationInterface::SetConstantPosition (leftNodes.Get (i), 10, (10 + 20*i)); 
    AnimationInterface::SetConstantPosition (onuNodes.Get (i), 160, (10 + 20*i)); 
    AnimationInterface::SetConstantPosition (rightNodes.Get (i), 200, (10 + 20*i));
  }

  AnimationInterface::SetNodeDescription (gatewayNode, "Router"); // Optional
  AnimationInterface::SetNodeDescription (oltNode, "OLT"); // Optional
  AnimationInterface::SetNodeDescription (onuNodes, "ONU"); // Optional
  AnimationInterface::SetNodeColor (gatewayNode, 0, 255, 0); // Optional
  AnimationInterface::SetNodeColor (oltNode, 255, 0, 0); // Optional
  AnimationInterface::SetNodeColor (onuNodes, 255, 0, 0); // Optional
  AnimationInterface::SetNodeColor (rightNodes, 0, 0, 255); // Optional
  AnimationInterface::SetNodeColor (leftNodes, 0, 0, 255); // Optional


  AnimationInterface anim (p_animFilename);
  //anim.SetStartTime (Seconds(p_appStartTime+2));
  //anim.SetStopTime (Seconds(p_simStopTime-2));

  Simulator::Stop(Seconds(p_simStopTime));
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout<<std::endl;
  
  return 0;
}

