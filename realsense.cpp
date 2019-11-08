#include <iostream>
#include <iomanip>
#include <map>
#include <utility>
#include <vector>
#include <librealsense2/rs.hpp>

namespace helper
{
    inline int get_user_selection (const std::string& promt_message)
    {
        std::cout << "\n" << promt_message;
        int input;
        std:: cin >> input;
        std::cout << std::endl;
        return input;
    }
}

void calc_transform(rs2_pose& pose_data, float mat[16])
{
    auto q = pose_data.rotation;
    auto t = pose_data.translation;
    // Set the matrix as column-major for convenient work with OpenGL and rotate by 180 degress (by negating 1st and 3rd columns)
    mat[0] = -(1 - 2 * q.y*q.y - 2 * q.z*q.z); mat[4] = 2 * q.x*q.y - 2 * q.z*q.w;     mat[8] = -(2 * q.x*q.z + 2 * q.y*q.w);      mat[12] = t.x;
    mat[1] = -(2 * q.x*q.y + 2 * q.z*q.w);     mat[5] = 1 - 2 * q.x*q.x - 2 * q.z*q.z; mat[9] = -(2 * q.y*q.z - 2 * q.x*q.w);      mat[13] = t.y;
    mat[2] = -(2 * q.x*q.z - 2 * q.y*q.w);     mat[6] = 2 * q.y*q.z + 2 * q.x*q.w;     mat[10] = -(1 - 2 * q.x*q.x - 2 * q.y*q.y); mat[14] = t.z;
    mat[3] = 0.0f;                             mat[7] = 0.0f;                          mat[11] = 0.0f;                             mat[15] = 1.0f;
}

class RUIT265Object
{
public:
    static rs2::device get_a_device()
    {
        rs2::context ctx;
        rs2::device_list devices = ctx.query_devices();
        rs2::device selected_device;
        if (devices.size() == 0) {
            std::cout << "No device connected, pleas connect a Realsense device" << std::endl;
            rs2::device_hub device_hub(ctx);
            selected_device = device_hub.wait_for_device();
        }
        else {
            std::cout << "Succees found the devices." << std::endl;
            int index = 0;
            for (rs2::device device : devices)
            {
                std::cout << " " << ++index << " : " << get_device_name(device) << std::endl;
            }
            uint32_t selected_device_index = helper::get_user_selection("Select a device by index: ");

            if (selected_device_index >= devices.size())
            {
                throw std::out_of_range("Select device index is out of range");
            }
            selected_device = devices[selected_device_index];
        }
        return selected_device;
    }

    static std::string get_device_name(const rs2::device& dev)
    {
        std::string name = "Unkonw device";
        if (dev.supports(RS2_CAMERA_INFO_NAME))
            name = dev.get_info(RS2_CAMERA_INFO_NAME);
        std::string sn = "########";
        if(dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
            sn = std::string("#") + dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        return name + " " + sn;
    }

    static std::string get_sensor_name(const rs2::sensor& sensor)
    {
        if (sensor.supports(RS2_CAMERA_INFO_NAME))
            return sensor.get_info(RS2_CAMERA_INFO_NAME);
        else
            return "Unkonw sensor";
    }

    static rs2_option get_sensor_option(const rs2::sensor& sensor)
    {
        std::cout << "Sensor supports the following options:\n" << std::endl;
        for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
        {
            rs2_option option_type = static_cast<rs2_option>(i);
            std::cout << "  " << i << ": " << option_type;
            if (sensor.supports(option_type))
            {
                std::cout << std::endl;
                const char* description = sensor.get_option_description(option_type);
                std::cout << "  Description :" << description << std::endl;
                float current_value = sensor.get_option(option_type);
                std::cout << "  Current Value : " << current_value << std::endl;
            }
            else
            {
                std::cout << "Is not supported" << std::endl;
            }
        }
        int select_sensor_option = helper::get_user_selection("Select an option by index: ");
        if (select_sensor_option >= static_cast<int>(RS2_OPTION_COUNT))
        {
            throw std::out_of_range("Select option is out of range");
        }
        return static_cast<rs2_option>(select_sensor_option);
    }

private:
};


int main()
{
    rs2::device dev;
    RUIT265Object obj;
    dev = obj.get_a_device();
    std::string deviceName;
    deviceName = obj.get_device_name(dev);
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
    pipe.start(cfg);

    auto frames = pipe.wait_for_frames();
    auto f = frames.first_or_default(RS2_STREAM_POSE);
    auto pose_data = f.as<rs2::pose_frame>().get_pose_data();
    float r[16];
    calc_transform(pose_data, r);  // r will be the transformatrix we want



}
