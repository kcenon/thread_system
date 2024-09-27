#include "file_job.h"

#ifdef USE_STD_FORMAT
#include <format>
#else
#include "fmt/format.h"
#endif

#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>

namespace log_module
{
	file_job::file_job(const std::string& title,
					   const std::string& message,
					   const uint32_t& max_lines,
					   const bool& use_backup)
		: job(nullptr, "file_job")
		, title_(title)
		, message_(message)
		, max_lines_(max_lines)
		, use_backup_(use_backup)
	{
	}

	auto file_job::do_work() -> std::tuple<bool, std::optional<std::string>>
	{
		if (message_.empty())
		{
			return { false, "empty message" };
		}

		try
		{
			const auto now = std::chrono::system_clock::now();
			const auto today = std::chrono::floor<std::chrono::days>(now);
			const auto year_month_day = std::chrono::year_month_day{ today };

			const int year = static_cast<int>(year_month_day.year());
			const unsigned month = static_cast<unsigned>(year_month_day.month());
			const unsigned day = static_cast<unsigned>(year_month_day.day());

			const std::string file_name =
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				("{}_{:04d}-{:02d}-{:02d}.log", title_, year, month, day);
			const std::string backup_name =
#ifdef USE_STD_FORMAT
				std::format
#else
				fmt::format
#endif
				("{}_{:04d}-{:02d}-{:02d}.backup", title_, year, month, day);

			if (max_lines_ == 0)
			{
				std::ofstream outfile(file_name, std::ios_base::out | std::ios_base::app);
				if (!outfile.is_open())
				{
					return { false, "error opening file" };
				}
				outfile << message_;
				return { true, std::nullopt };
			}

			std::vector<std::string> read_lines;
			if (std::filesystem::exists(file_name))
			{
				std::ifstream readfile(file_name);
				if (!readfile.is_open())
				{
					return { false, "error opening file" };
				}

				std::string line;
				while (std::getline(readfile, line))
				{
					read_lines.push_back(line);
				}
			}

			if (use_backup_ && read_lines.size() >= max_lines_)
			{
				std::ofstream backup_file(backup_name, std::ios_base::out | std::ios_base::app);
				if (backup_file.is_open())
				{
					for (size_t i = 0; i < read_lines.size() - max_lines_ + 1; ++i)
					{
						backup_file << read_lines[i] << '\n';
					}
				}
			}

			std::ofstream outfile(file_name, std::ios_base::out | std::ios_base::trunc);
			if (!outfile.is_open())
			{
				return { false, "error opening file" };
			}

			size_t start_index
				= (read_lines.size() > max_lines_ - 1) ? read_lines.size() - max_lines_ + 1 : 0;
			for (size_t i = start_index; i < read_lines.size(); ++i)
			{
				outfile << read_lines[i] << '\n';
			}

			outfile << message_;

			return { true, std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { false, std::string(e.what()) };
		}
		catch (...)
		{
			return { false, "unknown error" };
		}
	}
} // namespace log_module