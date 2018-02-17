#include "ros/ros.h"
#include "std_msgs/String.h"

#include <sstream>
#include "net.h"

#define TIME_OUT    1000
#define SYNC_FREQ   20    // frames between sync
#define FPS         10    // frames per second
#define TRIAL_COUNT 50    // trials for syncing

static conn_t cmdConn;
static conn_t syncConn;

uint64_t parse_long(const char bs[]);

void sync(char numTrials, const ros::Publisher & publisher) {
    uint64_t time;
    int status;

    const int recvSize = 8;
    char recvBuffer[recvSize];

    const int sendSize = 2;
    char sendBuffer[sendSize] = {3, numTrials}; //command, numTrials

    const int confSize = 1;
    char confBuffer[] = {1};
    uint64_t timeOffsets[numTrials];

    memset(recvBuffer, 0, recvSize);
    
    // Send initial request (command) to start time sync
    status = send_data(cmdConn, sendBuffer, sendSize);
    if (status == -1) ROS_INFO("Problem sending data.");
    
    for (int count = 0; count < numTrials; count++) {
        //Wait to receive 'bit' from RIO
        ROS_INFO("Waiting for 'bit'...");

        status = recv_data(syncConn, recvBuffer, 1, TIME_OUT);
        
        time = ros::Time::now().toNSec();
        
        if (status == -1) ROS_INFO("Problem receiving 'bit.'");
        if (status == -2) {
            ROS_INFO("Gave up waiting for 'bit.' Seems inevitable.");
            return;
        }
        ROS_INFO_STREAM("Received 'bit': " << (int) recvBuffer[0]);
        
        //Send confirmation to RIO
        status = send_data(syncConn, confBuffer, confSize);
        if (status == -1) ROS_INFO("Failed to confirm that confirmation packet was sent and confirmed.");

        //Wait to receive time from RIO
        ROS_INFO("Waiting for RIO time packet...");
        status = recv_data(syncConn, recvBuffer, recvSize, TIME_OUT);
        if (status == -1) ROS_INFO("Problem receiving RIO time packet.");
        if (status == -2) {
            ROS_INFO("Where's my friggin' time packet??!!");
            return;
        }
        ROS_INFO_STREAM("Received data: {" 
            << (int)recvBuffer[0] << ", "
            << (int)recvBuffer[1] << ", "
            << (int)recvBuffer[2] << ", "
            << (int)recvBuffer[3] << ", "
            << (int)recvBuffer[4] << ", "
            << (int)recvBuffer[5] << ", "
            << (int)recvBuffer[6] << ", "
            << (int)recvBuffer[7] << "}");

        //Send second confirmation to RIO
        status = send_data(syncConn, confBuffer, confSize);
        if (status == -1) ROS_INFO("Failed to confirm that confirmation packet was sent and confirmed.");

        // compute syncing offset
        uint64_t rio_time = parse_long(recvBuffer);
        timeOffsets[count] = time - rio_time;


        ROS_INFO_STREAM("Got RIO timestamp: " << rio_time);
        ROS_INFO_STREAM("Computed Diff: " << timeOffsets[count]);

    }

    // get the minimal offset
    uint8_t min = timeOffsets[0];
    for (int i = 0; i < numTrials; i++) {
        min = timeOffsets[i] < min ? timeOffsets[i] : min;
        if(i > 0)
            ROS_INFO_STREAM(i << ". Difference from last offset: " 
                << (int64_t) (timeOffsets[i] - timeOffsets[i-1]) << " ns");
    }
    // pack and ship offset to everyone
    std::stringstream ss;
    ss << min;
    std_msgs::String msg;
    msg.data = ss.str();
    publisher.publish(msg);
}

int main(int argc, char ** argv) {
    int status;
    ros::init(argc, argv, "sync_node");

    ros::NodeHandle node;
    ros::Publisher publisher = node.advertise<std_msgs::String>("rio_time_offset", 10);

    ros::Rate loop_rate(FPS);

    ROS_INFO("Starting client...");
    status = start_client(cmdConn, "10.21.54.2", 5800);
    
    if(status == -1) ROS_INFO("Problem starting client.");

    ROS_INFO("Starting server...");
    status = start_server(syncConn, 22222);
    if(status == -1) ROS_INFO("Problem starting server.");
    
    for (int i = 0; ros::ok(); i = (++i) % SYNC_FREQ) {
        if (i == 0) sync(TRIAL_COUNT, publisher);
        ros::spinOnce();
        loop_rate.sleep();
    }

    close_conn(cmdConn);
    close_conn(syncConn);
}

uint64_t parse_long(const char bs[]) {
    uint64_t out = 0;
    for(int i = 7; i >= 0; i--) {
        out = out << 8;
        out |= bs[i];
    }
    return out;
}
