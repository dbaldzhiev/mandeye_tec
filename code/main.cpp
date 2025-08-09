
#include <chrono>
#include <json.hpp>
#include <ostream>
#include <thread>

#include "compilation_constants.h"
#include "gnss.h"
#include "publisher.h"
#include "save_laz.h"
#include <FileSystemClient.h>
#include <LivoxClient.h>
#include <chrono>
#include <fstream>
#include "utils/logger.h"
#include <string>

#define MANDEYE_LIVOX_LISTEN_IP "192.168.1.5"
#define MANDEYE_REPO "/media/usb/"
#define SERVER_PORT 8003

namespace utils
{
std::string getEnvString(const std::string& env, const std::string& def);
bool getEnvBool(const std::string& env, bool def);
int getEnvInt(const std::string& env, int def);
} // namespace utils

struct MandeyeConfig
{
        std::string livox_interface_ip = MANDEYE_LIVOX_LISTEN_IP;
        std::string repository_path = MANDEYE_REPO;
        int server_port = SERVER_PORT;
};

MandeyeConfig LoadConfig();
static MandeyeConfig g_config;

namespace mandeye
{
enum class States
{
	WAIT_FOR_RESOURCES = -10,
	IDLE = 0,
	STARTING_SCAN = 10,
	SCANNING = 20,
	STOPPING = 30,
	STOPPING_STAGE_1 = 31,
	STOPPING_STAGE_2 = 32,
	STOPPED = 40,
	STARTING_STOP_SCAN = 100,
	STOP_SCAN_IN_PROGRESS = 150,
	STOP_SCAN_IN_INITIAL_PROGRESS = 160,
	STOPING_STOP_SCAN = 190,
	LIDAR_ERROR = 200,
	USB_IO_ERROR = 210,
};

const std::map<States, std::string> StatesToString{
	{States::WAIT_FOR_RESOURCES, "WAIT_FOR_RESOURCES"},
	{States::IDLE, "IDLE"},
	{States::STARTING_SCAN, "STARTING_SCAN"},
	{States::SCANNING, "SCANNING"},
	{States::STOPPING, "STOPPING"},
	{States::STOPPING_STAGE_1, "STOPPING_STAGE_1"},
	{States::STOPPING_STAGE_2, "STOPPING_STAGE_2"},
	{States::STOPPED, "STOPPED"},
	{States::STARTING_STOP_SCAN, "STARTING_STOP_SCAN"},
	{States::STOP_SCAN_IN_PROGRESS, "STOP_SCAN_IN_PROGRESS"},
	{States::STOP_SCAN_IN_INITIAL_PROGRESS, "STOP_SCAN_IN_INITIAL_PROGRESS"},
	{States::STOPING_STOP_SCAN, "STOPING_STOP_SCAN"},
	{States::LIDAR_ERROR, "LIDAR_ERROR"},
	{States::USB_IO_ERROR, "USB_IO_ERROR"},
};

std::atomic<bool> isRunning{true};
std::atomic<bool> isLidarError{false};
std::mutex livoxClientPtrLock;
std::shared_ptr<LivoxClient> livoxCLientPtr;
std::shared_ptr<GNSSClient> gnssClientPtr;
std::shared_ptr<FileSystemClient> fileSystemClientPtr;
std::shared_ptr<Publisher> publisherPtr;
mandeye::LazStats lastFileSaveStats;
double usbWriteSpeed10Mb = 0.0;
double usbWriteSpeed1Mb = 0.0;
mandeye::States app_state{mandeye::States::WAIT_FOR_RESOURCES};

using json = nlohmann::json;

std::string produceReport(bool reportUSB = true)
{
	json j;
	j["name"] = "Mandeye";
	j["hash"] = GIT_HASH;
	j["version"] = MANDEYE_VERSION;
	j["hardware"] = MANDEYE_HARDWARE_HEADER;
	j["arch"] = SYSTEM_ARCH;
	j["state"] = StatesToString.at(app_state);
	if(livoxCLientPtr)
	{
		j["livox"] = livoxCLientPtr->produceStatus();
	}
	else
	{
		j["livox"] = {};
	}

	j["fs_benchmark"]["write_speed_10mb"] = std::round(usbWriteSpeed10Mb * 100) / 100.0;
	j["fs_benchmark"]["write_speed_1mb"] = std::round(usbWriteSpeed1Mb * 100) / 100.0;

	if(fileSystemClientPtr && reportUSB)
	{
		j["fs"] = fileSystemClientPtr->produceStatus();
	}
	if(gnssClientPtr)
	{
		j["gnss"] = gnssClientPtr->produceStatus();
	}
	else
	{
		j["gnss"] = {};
	}

	j["lastLazStatus"] = lastFileSaveStats.produceStatus();

	std::ostringstream s;
	s << std::setw(4) << j;
	return s.str();
}

bool StartScan()
{
	if(app_state == States::IDLE || app_state == States::STOPPED)
	{
		app_state = States::STARTING_SCAN;
		return true;
	}
	return false;
}
bool StopScan()
{
	if(app_state == States::SCANNING)
	{
		app_state = States::STOPPING;
		return true;
	}
	return false;
}

using namespace std::chrono_literals;

bool TriggerStopScan()
{
	if(app_state == States::IDLE || app_state == States::STOPPED)
	{
		app_state = States::STARTING_STOP_SCAN;
		return true;
	}
	return false;
}

std::chrono::steady_clock::time_point stoppingStage1StartDeadline;
std::chrono::steady_clock::time_point stoppingStage1StartDeadlineChangeLed;
std::chrono::steady_clock::time_point stoppingStage2StartDeadline;
std::chrono::steady_clock::time_point stoppingStage2StartDeadlineChangeLed;

bool TriggerContinousScanning()
{
	if(app_state == States::IDLE || app_state == States::STOPPED)
	{

		// intiliaze duration count
		if(livoxCLientPtr)
		{
			livoxCLientPtr->initializeDuration();
		}

		app_state = States::STARTING_SCAN;
		return true;
	}
	else if(app_state == States::SCANNING)
	{
#ifdef MANDEYE_COUNTINOUS_SCANNING_STOP_1_CLICK
		app_state = States::STOPPING;
		return true;
#endif //MANDEYE_COUNTINOUS_SCANNING_STOP_1_CLICK
		app_state = States::STOPPING_STAGE_1;
		//stoppingStage1Start = std::chrono::steady_clock::now();
		stoppingStage1StartDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);

		stoppingStage1StartDeadlineChangeLed = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

		return true;
	}
	else if(app_state == States::STOPPING_STAGE_1)
	{

		const auto now = std::chrono::steady_clock::now();

		if(now < stoppingStage1StartDeadline)
		{
			//stoppingStage2Start = std::chrono::steady_clock::now();
			stoppingStage2StartDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2000);

			stoppingStage2StartDeadlineChangeLed = std::chrono::steady_clock::now() + std::chrono::milliseconds(25);

			app_state = States::STOPPING_STAGE_2;
		}

		return true;
	}
	else if(app_state == States::STOPPING_STAGE_2)
	{
		const auto now = std::chrono::steady_clock::now();
		if(now < stoppingStage2StartDeadline)
		{
			app_state = States::STOPPING;
		}
		return true;
	}
	return false;
}

void savePointcloudData(LivoxPointsBufferPtr buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];

	const auto start = std::chrono::steady_clock::now();
	snprintf(lidarName, 256, "lidar%04d.laz", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
    LOG_INFO("Savig lidar buffer of size " + std::to_string(buffer->size()) + " to " + lidarFilePath.string());
	auto saveStatus = saveLaz(lidarFilePath.string(), buffer);

	system("sync");
	const auto end = std::chrono::steady_clock::now();
	const std::chrono::duration<float> elapsed_seconds = end - start;
	if(saveStatus)
	{
		saveStatus->m_saveDurationSec2 = elapsed_seconds.count();
		mandeye::lastFileSaveStats = *saveStatus;
	}
	return;
}

void saveLidarList(const std::unordered_map<uint32_t, std::string>& lidars, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "lidar%04d.sn", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
    LOG_INFO("Savig lidar list of size " + std::to_string(lidars.size()) + " to " + lidarFilePath.string());

	std::ofstream lidarStream(lidarFilePath);
	for(const auto& [id, sn] : lidars)
	{
		lidarStream << id << " " << sn << "\n";
	}
	system("sync");
	return;
}

void saveStatusData(const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char statusName[256];
	snprintf(statusName, 256, "status%04d.json", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(statusName);
    LOG_INFO(std::string("Savig status to ") + lidarFilePath.string());
	std::ofstream lidarStream(lidarFilePath);
	lidarStream << produceReport(false);
	system("sync");
}

void saveImuData(LivoxIMUBufferPtr buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "imu%04d.csv", chunk);
	std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
    LOG_INFO("Savig imu buffer of size " + std::to_string(buffer->size()) + " to " + lidarFilePath.string());
	std::ofstream lidarStream(lidarFilePath.c_str());
	lidarStream << "timestamp gyroX gyroY gyroZ accX accY accZ imuId timestampUnix\n";
	std::stringstream ss;

	for(const auto& p : *buffer)
	{
		if(p.timestamp > 0)
		{
			ss << p.timestamp << " " << p.point.gyro_x << " " << p.point.gyro_y << " " << p.point.gyro_z << " " << p.point.acc_x << " "
			   << p.point.acc_y << " " << p.point.acc_z << " " << p.laser_id << " " << p.epoch_time << "\n";
		}
	}
	lidarStream << ss.rdbuf();

	lidarStream.close();
	system("sync");
	return;
}

void saveGnssData(std::deque<std::string>& buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "gnss%04d.gnss", chunk);
        std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
        LOG_INFO("Savig gnss buffer of size " + std::to_string(buffer.size()) + " to " + lidarFilePath.string());
	std::ofstream lidarStream(lidarFilePath.c_str());
	std::stringstream ss;

	for(const auto& p : buffer)
	{
		ss << p;
	}
	lidarStream << ss.rdbuf();

	lidarStream.close();
	system("sync");
	return;
}

void saveGnssRawData(std::deque<std::string>& buffer, const std::string& directory, int chunk)
{
	using namespace std::chrono_literals;
	char lidarName[256];
	snprintf(lidarName, 256, "gnss%04d.nmea", chunk);
        std::filesystem::path lidarFilePath = std::filesystem::path(directory) / std::filesystem::path(lidarName);
        LOG_INFO("Savig gnss raw buffer of size " + std::to_string(buffer.size()) + " to " + lidarFilePath.string());
	std::ofstream lidarStream(lidarFilePath.c_str());
	std::stringstream ss;

	for(const auto& p : buffer)
	{
		ss << p;
	}
	lidarStream << ss.rdbuf();

	lidarStream.close();
	system("sync");
	return;
}

void stateWatcher()
{
	using namespace std::chrono_literals;
	std::chrono::steady_clock::time_point chunkStart = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point stopScanDeadline = std::chrono::steady_clock::now();
	std::chrono::steady_clock::time_point stopScanInitialDeadline = std::chrono::steady_clock::now();
	States oldState = States::IDLE;
	std::string continousScanDirectory;
	std::string stopScanDirectory;
	int chunksInExperimentCS{0};
	int chunksInExperimentSS{0};

	int id_manifest = 0;
	if(stopScanDirectory.empty() && fileSystemClientPtr)
	{
		if(!fileSystemClientPtr->CreateDirectoryForStopScans(stopScanDirectory, id_manifest))
		{
			app_state = States::USB_IO_ERROR;
		}
	}
	if(stopScanDirectory.empty())
	{
		app_state = States::USB_IO_ERROR;
	}

	if(!fileSystemClientPtr->CreateDirectoryForContinousScanning(continousScanDirectory, id_manifest))
	{
		app_state = States::USB_IO_ERROR;
	}
	if(fileSystemClientPtr)
	{
#ifdef MANDEYE_BENCHMARK_WRITE_SPEED
            LOG_INFO("Benchmarking write speed");
		mandeye::usbWriteSpeed10Mb = fileSystemClientPtr->BenchmarkWriteSpeed("benchmark10.bin", 10);
		mandeye::usbWriteSpeed1Mb = fileSystemClientPtr->BenchmarkWriteSpeed("benchmark1.bin", 1);
            LOG_INFO("Benchmarking write speed done");
#endif
	}

	while(isRunning)
	{
		if(mandeye::publisherPtr)
		{
			mandeye::publisherPtr->SetMode(StatesToString.at(app_state));
		}
		if(oldState != app_state)
		{
                    LOG_INFO("State transtion from " + StatesToString.at(oldState) + " to " + StatesToString.at(app_state));
		}
		oldState = app_state;

                if(app_state == States::LIDAR_ERROR)
                {
                        std::this_thread::sleep_for(1000ms);
                        LOG_ERROR("app_state == States::LIDAR_ERROR");
                }
                if(app_state == States::USB_IO_ERROR)
                {
                        std::this_thread::sleep_for(1000ms);
                        LOG_ERROR("app_state == States::USB_IO_ERROR");
                }
                else if(app_state == States::WAIT_FOR_RESOURCES)
                {
                        std::this_thread::sleep_for(100ms);
                        std::lock_guard<std::mutex> l1(livoxClientPtrLock);
                        if(mandeye::fileSystemClientPtr)
                        {
                                app_state = States::IDLE;
                        }
                        if(mandeye::publisherPtr)
                        {
                                mandeye::publisherPtr->SetWorkingDirectory(stopScanDirectory, continousScanDirectory);
                        }
                }
                else if(app_state == States::IDLE)
                {
                        if(isLidarError)
                        {
                                app_state = States::LIDAR_ERROR;
                        }
                        std::this_thread::sleep_for(100ms);
                }
                else if(app_state == States::STARTING_SCAN)
                {
                        //chunksInExperiment = 0;
                        if(livoxCLientPtr)
                        {
                                livoxCLientPtr->startLog();
                                if(gnssClientPtr)
                                {
                                        gnssClientPtr->startLog();
                                }
                                app_state = States::SCANNING;
                        }
                        // create directory
			//if(!fileSystemClientPtr->CreateDirectoryForExperiment(continousScanDirectory)){
			//	app_state = States::USB_IO_ERROR;
			//}
			chunkStart = std::chrono::steady_clock::now();
		}
                else if(app_state == States::SCANNING || app_state == States::STOPPING_STAGE_1 || app_state == States::STOPPING_STAGE_2)
                {
                        const auto now = std::chrono::steady_clock::now();
                        if(now - chunkStart > std::chrono::seconds(10) && app_state == States::SCANNING)
                        {
                                chunkStart = std::chrono::steady_clock::now();

                                auto [lidarBuffer, imuBuffer] = livoxCLientPtr->retrieveData();
                                std::deque<std::string> gnssBuffer;
                                std::deque<std::string> gnssRawBuffer;

                                if(gnssClientPtr)
                                {
                                        gnssBuffer = gnssClientPtr->retrieveData();
                                        gnssRawBuffer = gnssClientPtr->retrieveRawData();
                                }
                                if(continousScanDirectory == "")
                                {
                                        app_state = States::USB_IO_ERROR;
                                }
                                else
                                {
                                        savePointcloudData(lidarBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                        saveImuData(imuBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                        saveStatusData(continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                        auto lidarList = livoxCLientPtr->getSerialNumberToLidarIdMapping();
                                        saveLidarList(lidarList, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                        if(gnssClientPtr)
                                        {
                                                saveGnssData(gnssBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                                saveGnssRawData(gnssRawBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                        }
                                        chunksInExperimentCS++;
                                }
                        }

                        if(app_state == States::STOPPING_STAGE_1)
                        {
                                LOG_INFO("app_state == States::STOPPING_STAGE_1");

                                const auto now = std::chrono::steady_clock::now();

                                if(now > stoppingStage1StartDeadline)
                                {
                                        app_state = States::SCANNING;
                                }
                        }

                        if(app_state == States::STOPPING_STAGE_2)
                        {
                                LOG_INFO("app_state == States::STOPPING_STAGE_2");

                                const auto now = std::chrono::steady_clock::now();

                                if(now > stoppingStage2StartDeadline)
                                {
                                        app_state = States::SCANNING;
                                }
                        }

			std::this_thread::sleep_for(20ms);
		}
                else if(app_state == States::STOPPING)
                {
                        auto [lidarBuffer, imuBuffer] = livoxCLientPtr->retrieveData();
                        std::deque<std::string> gnssData;
                        livoxCLientPtr->stopLog();
                        if(gnssClientPtr)
                        {
                                gnssData = gnssClientPtr->retrieveData();
                                gnssClientPtr->stopLog();
                        }

                        if(continousScanDirectory.empty())
                        {
                                app_state = States::USB_IO_ERROR;
                        }
                        else
                        {
                                savePointcloudData(lidarBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                saveImuData(imuBuffer, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                saveStatusData(continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                auto lidarList = livoxCLientPtr->getSerialNumberToLidarIdMapping();
                                saveLidarList(lidarList, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                if(gnssClientPtr)
                                {
                                        saveGnssData(gnssData, continousScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
                                }
                                chunksInExperimentCS++;
                                app_state = States::IDLE;
                        }
                }
		else if(app_state == States::STARTING_STOP_SCAN)
		{
			//if (stopScanDirectory.empty() && fileSystemClientPtr)
			//{
			//	if(!fileSystemClientPtr->CreateDirectoryForStopScans(stopScanDirectory)){
			//		app_state = States::USB_IO_ERROR;
			//	}
			//}
			//if(stopScanDirectory.empty()){
			//	app_state = States::USB_IO_ERROR;
			//}

			stopScanInitialDeadline = std::chrono::steady_clock::now();
			stopScanInitialDeadline += std::chrono::milliseconds(5000);

			stopScanDeadline = stopScanInitialDeadline + std::chrono::milliseconds(30000);

			app_state = States::STOP_SCAN_IN_INITIAL_PROGRESS;
		}
		else if(app_state == States::STOP_SCAN_IN_INITIAL_PROGRESS)
		{
			const auto now = std::chrono::steady_clock::now();

                        if(now < stopScanInitialDeadline)
                        {
                                std::this_thread::sleep_for(200ms);
                        }
                        else
                        {
                                if(livoxCLientPtr)
                                {
                                        livoxCLientPtr->startLog();
                                }
                                if(gnssClientPtr)
                                {
                                        gnssClientPtr->startLog();
                                }
                                app_state = States::STOP_SCAN_IN_PROGRESS;
                        }
		}
		else if(app_state == States::STOP_SCAN_IN_PROGRESS)
		{
			const auto now = std::chrono::steady_clock::now();
			if(now > stopScanDeadline)
			{
				app_state = States::STOPING_STOP_SCAN;
			}
		}
		else if(app_state == States::STOPING_STOP_SCAN)
		{
			auto [lidarBuffer, imuBuffer] = livoxCLientPtr->retrieveData();
			std::deque<std::string> gnssData;
			livoxCLientPtr->stopLog();
			if(gnssClientPtr)
			{
				gnssData = gnssClientPtr->retrieveData();
				gnssClientPtr->stopLog();
			}
			if(stopScanDirectory.empty())
			{
				app_state = States::USB_IO_ERROR;
			}
			else
			{
				savePointcloudData(lidarBuffer, stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				saveImuData(imuBuffer, stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				saveStatusData(stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				auto lidarList = livoxCLientPtr->getSerialNumberToLidarIdMapping();
				saveLidarList(lidarList, stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				if(gnssClientPtr)
				{
					saveGnssData(gnssData, stopScanDirectory, chunksInExperimentCS + chunksInExperimentSS);
				}
				chunksInExperimentSS++;
				app_state = States::IDLE;
			}
		}
	}
}
} // namespace mandeye

namespace utils
{
std::string getEnvString(const std::string& env, const std::string& def)
{
	const char* env_p = std::getenv(env.c_str());
	if(env_p == nullptr)
	{
		return def;
	}
	return std::string{env_p};
}

bool getEnvBool(const std::string& env, bool def)
{
	const char* env_p = std::getenv(env.c_str());
	if(env_p == nullptr)
	{
		return def;
	}
	if(strcmp("1", env_p) == 0 || strcmp("true", env_p) == 0)
	{
		return true;
	}
        return false;
}

int getEnvInt(const std::string& env, int def)
{
        const char* env_p = std::getenv(env.c_str());
        if(env_p == nullptr)
        {
                return def;
        }
        try
        {
                return std::stoi(env_p);
        }
        catch(...)
        {
                return def;
        }
}
} // namespace utils

MandeyeConfig LoadConfig()
{
        MandeyeConfig cfg;
        cfg.livox_interface_ip = utils::getEnvString("MANDEYE_LIVOX_LISTEN_IP", cfg.livox_interface_ip);
        cfg.repository_path = utils::getEnvString("MANDEYE_REPO", cfg.repository_path);
        cfg.server_port = utils::getEnvInt("SERVER_PORT", cfg.server_port);

        std::ifstream f("config/mandeye_config.json");
        if(f)
        {
                try
                {
                        nlohmann::json j;
                        f >> j;
                        if(j.contains("livox_interface_ip") && j["livox_interface_ip"].is_string())
                        {
                                cfg.livox_interface_ip = j["livox_interface_ip"].get<std::string>();
                        }
                        if(j.contains("repository_path") && j["repository_path"].is_string())
                        {
                                cfg.repository_path = j["repository_path"].get<std::string>();
                        }
                        if(j.contains("server_port") && j["server_port"].is_number_integer())
                        {
                                cfg.server_port = j["server_port"].get<int>();
                        }
                }
                catch(...)
                {
                        // ignore parse errors
                }
        }
        return cfg;
}

std::thread thLivox;
std::thread thStateMachine;

void InitProgram()
{
    g_config = LoadConfig();
    mandeye::fileSystemClientPtr = std::make_shared<mandeye::FileSystemClient>(g_config.repository_path);
    thLivox = std::thread([&]() {
        {
            std::lock_guard<std::mutex> l1(mandeye::livoxClientPtrLock);
            mandeye::livoxCLientPtr = std::make_shared<mandeye::LivoxClient>();
        }
        if(!mandeye::livoxCLientPtr->startListener(g_config.livox_interface_ip)) {
            mandeye::isLidarError.store(true);
        }
        const std::string portName = "";
        const LibSerial::BaudRate baud = LibSerial::BaudRate::BAUD_9600;
        if(!portName.empty()) {
            mandeye::gnssClientPtr = std::make_shared<mandeye::GNSSClient>();
            mandeye::gnssClientPtr->SetTimeStampProvider(mandeye::livoxCLientPtr);
            mandeye::gnssClientPtr->startListener(portName, baud);
        }
        mandeye::publisherPtr = std::make_shared<mandeye::Publisher>(g_config.server_port);
        mandeye::publisherPtr->SetTimeStampProvider(mandeye::livoxCLientPtr);
    });

    thStateMachine = std::thread([&]() { mandeye::stateWatcher(); });
}

void ShutdownProgram()
{
    mandeye::isRunning.store(false);
    if(thStateMachine.joinable()) {
        thStateMachine.join();
    }
    if(thLivox.joinable()) {
        thLivox.join();
    }
}

#ifndef MANDEYE_LIBRARY
int main(int argc, char** argv)
{
    mandeye::Logger::Instance().SetLogFile("logs/mandeye.log");
    LOG_INFO(std::string("program: ") + argv[0] + " " + MANDEYE_VERSION + " " + MANDEYE_HARDWARE_HEADER);

    InitProgram();

    while(mandeye::isRunning)
    {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1000ms);
        char ch = std::getchar();
        if(ch == 'q')
        {
            mandeye::isRunning.store(false);
        }
        LOG_INFO("Press q -> quit, s -> start scan , e -> end scan");

        if(ch == 's')
        {
            if(mandeye::StartScan())
            {
                LOG_INFO("start scan success!");
            }
        }
        else if(ch == 'e')
        {
            if(mandeye::StopScan())
            {
                LOG_INFO("stop scan success!");
            }
        }
    }

    ShutdownProgram();
    LOG_INFO("Done");
    return 0;
}
#endif // MANDEYE_LIBRARY

extern "C" bool StartScan()
{
    return mandeye::StartScan();
}

extern "C" bool StopScan()
{
    return mandeye::StopScan();
}

extern "C" bool TriggerStopScan()
{
    return mandeye::TriggerStopScan();
}

extern "C" const char* produceReport(bool reportUSB)
{
    static std::string p;
    p = mandeye::produceReport(reportUSB);
    return p.c_str();
}

extern "C" void Init()
{
    InitProgram();
}

extern "C" void Shutdown()
{
    ShutdownProgram();
}
