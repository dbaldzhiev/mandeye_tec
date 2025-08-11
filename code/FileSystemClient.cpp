#include "FileSystemClient.h"
#include "utils/logger.h"
#include "save_laz.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdbool.h>
#include <string>
#include <unistd.h>
namespace mandeye
{

FileSystemClient::FileSystemClient(const std::string& repository)
	: m_repository(repository)
{
	m_nextId = GetIdFromManifest();
}
nlohmann::json FileSystemClient::produceStatus()
{
	nlohmann::json data;
	data["FileSystemClient"]["repository"] = m_repository;

	float free_mb{-1.f};
	bool mounted{false};
	bool writable{false};

	try
	{
		free_mb = CheckAvailableSpace();
		mounted = free_mb >= 0.f;
	}
	catch(std::filesystem::filesystem_error& e)
	{
		data["FileSystemClient"]["error"] = e.what();
	}

	try
	{
		writable = GetIsWritable();
	}
	catch(std::filesystem::filesystem_error& e)
	{
		data["FileSystemClient"]["error"] = e.what();
	}

	data["FileSystemClient"]["free_megabytes"] = free_mb;
	data["FileSystemClient"]["free_str"] = ConvertToText(free_mb);
	data["FileSystemClient"]["benchmarkWriteSpeed"] = m_benchmarkWriteSpeed;
	data["FileSystemClient"]["mounted"] = mounted;
	data["FileSystemClient"]["writable"] = writable;

	try
	{
		data["FileSystemClient"]["m_nextId"] = m_nextId;
	}
	catch(std::filesystem::filesystem_error& e)
	{
		data["FileSystemClient"]["error"] = e.what();
	}

	try
	{
		data["FileSystemClient"]["dirs"] = GetDirectories();
	}
	catch(std::filesystem::filesystem_error& e)
	{
		data["FileSystemClient"]["error"] = e.what();
	}
	return data;
}

//! Test is writable
float FileSystemClient::CheckAvailableSpace()
{
	std::error_code ec;
	const std::filesystem::space_info si = std::filesystem::space(m_repository, ec);
	if(ec.value() == 0)
	{
		const float f = static_cast<float>(si.free) / (1024 * 1024);
		return std::round(f);
	}
	return -1.f;
}

std::string FileSystemClient::ConvertToText(float mb)
{
	std::stringstream tmp;
	tmp << std::setprecision(1) << std::fixed << mb / 1024 << "GB";
	return tmp.str();
}

int32_t FileSystemClient::GetIdFromManifest()
{
	std::filesystem::path versionfn = std::filesystem::path(m_repository) / std::filesystem::path(versionFilename);
	std::ofstream versionOFstream;
	versionOFstream.open(versionfn.c_str());
	versionOFstream << "Version 0.6-dev" << std::endl;

	std::filesystem::path manifest = std::filesystem::path(m_repository) / std::filesystem::path(manifestFilename);
	std::unique_lock<std::mutex> lck(m_mutex);

	std::ifstream manifestFstream;
	manifestFstream.open(manifest.c_str());
	if(manifestFstream.good() && manifestFstream.is_open())
	{
		uint32_t id{0};
		manifestFstream >> id;
		return (id++);
	}
	std::ofstream manifestOFstream;
	manifestOFstream.open(manifest.c_str());
	if(manifestOFstream.good() && manifestOFstream.is_open())
	{
		uint32_t id{0};
		manifestOFstream << id << std::endl;
		return (id++);
	}
	//

	return -1;
}

int32_t FileSystemClient::GetNextIdFromManifest()
{
	std::filesystem::path manifest = std::filesystem::path(m_repository) / std::filesystem::path(manifestFilename);
	int32_t id = GetIdFromManifest();
	id++;
	m_nextId = id;
	std::ofstream manifestOFstream;
	manifestOFstream.open(manifest.c_str());
	if(manifestOFstream.good() && manifestOFstream.is_open())
	{
		manifestOFstream << id << std::endl;
		return id;
	}
	return id;
}

bool FileSystemClient::CreateDirectoryForContinousScanning(std::string& writable_dir, const int& id_manifest)
{
	std::string ret;

	if(GetIsWritable())
	{
		//auto id = GetNextIdFromManifest();
		auto id = id_manifest;
		char dirName[256];
		snprintf(dirName, 256, "continousScanning_%04d", id);
		std::filesystem::path newDirPath = std::filesystem::path(m_repository) / std::filesystem::path(dirName);
		LOG_INFO("Creating directory " + newDirPath.string());
		std::error_code ec;
		std::filesystem::create_directories(newDirPath, ec);
		m_error = ec.message();
		if(ec.value() == 0)
		{
			if(!newDirPath.string().empty())
			{
				writable_dir = newDirPath.string();
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool FileSystemClient::CreateDirectoryForStopScans(std::string& writable_dir, int& id_manifest)
{
	std::string ret;

	if(GetIsWritable())
	{
		id_manifest = GetNextIdFromManifest() - 1;
		char dirName[256];
		snprintf(dirName, 256, "stopScans_%04d", id_manifest);
		std::filesystem::path newDirPath = std::filesystem::path(m_repository) / std::filesystem::path(dirName);
		LOG_INFO("Creating directory " + newDirPath.string());
		std::error_code ec;
		std::filesystem::create_directories(newDirPath, ec);
		m_error = ec.message();
		if(ec.value() == 0)
		{
			if(!newDirPath.string().empty())
			{
				writable_dir = newDirPath.string();
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

std::vector<std::string> FileSystemClient::GetDirectories()
{
	std::unique_lock<std::mutex> lck(m_mutex);
	std::vector<std::string> fn;

	for(const auto& entry : std::filesystem::recursive_directory_iterator(m_repository))
	{
		if(entry.is_regular_file())
		{
			auto size = std::filesystem::file_size(entry);
			float fsize = static_cast<float>(size) / (1024 * 1024);
			fn.push_back(entry.path().string() + " " + std::to_string(fsize) + " Mb");
		}
		else
		{
			fn.push_back(entry.path().string());
		}
	}
	std::sort(fn.begin(), fn.end());
	return fn;
}

bool FileSystemClient::GetIsWritable()
{
	std::error_code ec;

	if(!std::filesystem::exists(m_repository, ec) || !std::filesystem::is_directory(m_repository, ec))
	{
		return false;
	}

	std::filesystem::space(m_repository, ec);
	if(ec)
	{
		return false;
	}

	return (access(m_repository.c_str(), W_OK) == 0);
}

double FileSystemClient::BenchmarkWriteSpeed(const std::string& filename, size_t fileSizeMB)
{
	const size_t bufferSize = 1024 * 1024; // 1 MB buffer
	std::vector<char> buffer(bufferSize, 0xAA);
	std::filesystem::path fileName = std::filesystem::path(m_repository) / std::filesystem::path(filename);
	std::ofstream out(fileName.string(), std::ios::binary);
	if(!out)
	{
		LOG_ERROR("Failed to open file for writing");
		return 0.0;
	}

	auto start = std::chrono::high_resolution_clock::now();
	for(size_t i = 0; i < fileSizeMB; ++i)
	{
		out.write(buffer.data(), bufferSize);
	}
	out.close();
	auto end = std::chrono::high_resolution_clock::now();
	system("sync");
	std::chrono::duration<double> elapsed = end - start;
	double mbps = fileSizeMB / elapsed.count();

	LOG_INFO("Wrote " + std::to_string(fileSizeMB) + " MB in " + std::to_string(elapsed.count()) + " seconds (" + std::to_string(mbps) + " MB/s)");
	// clear file
	// Remove the file after benchmarking
	//	std::error_code ec;
	//	std::filesystem::remove(fileName, ec);
	//	if (ec) {
	//		std::cerr << "Failed to remove benchmark file: " << ec.message() << std::endl;
	//	}
	return mbps;
}

bool FileSystemClient::ShouldRotate(size_t pointCount, double elapsedSec, int chunkSizeMb, int chunkDurationSec,
                                   float &remainingSizeMb, float &remainingTimeSec)
{
        const float currentSizeMb = static_cast<float>(pointCount) *
                                   (LAS_POINT_RECORD_SIZE_BYTES / (1024.f * 1024.f));
        remainingSizeMb = static_cast<float>(chunkSizeMb) - currentSizeMb;
        remainingTimeSec = static_cast<float>(chunkDurationSec) - static_cast<float>(elapsedSec);
        return remainingSizeMb <= 0.f || remainingTimeSec <= 0.f;
}

} // namespace mandeye
