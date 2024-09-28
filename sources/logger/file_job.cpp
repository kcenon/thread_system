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
			auto [file_name, backup_name] = generate_file_name();

			if (max_lines_ == 0)
			{
				if (!append_lines(file_name, { message_ }))
				{
					return { false, "error opening file" };
				}

				return { true, std::nullopt };
			}

			auto lines = read_lines(file_name);

			if (use_backup_ && lines.size() >= max_lines_)
			{
				std::vector<std::string> backup_lines(lines.begin(), lines.begin() + lines.size()
																		 - max_lines_ + 1);
				if (!append_lines(backup_name, backup_lines))
				{
					return { false, "error opening backup file" };
				}
			}

			std::ofstream outfile(file_name, std::ios_base::out | std::ios_base::trunc);
			if (!outfile.is_open())
			{
				return { false, "error opening file" };
			}

			size_t start_index
				= (lines.size() > max_lines_ - 1) ? lines.size() - max_lines_ + 1 : 0;
			for (size_t i = start_index; i < lines.size(); ++i)
			{
				outfile << lines[i];
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

	auto file_job::generate_file_name() -> std::tuple<std::string, std::string>
	{
		const auto now = std::chrono::system_clock::now();
		const auto today = std::chrono::floor<std::chrono::days>(now);
		const auto year_month_day = std::chrono::year_month_day{ today };

		const int year = static_cast<int>(year_month_day.year());
		const unsigned month = static_cast<unsigned>(year_month_day.month());
		const unsigned day = static_cast<unsigned>(year_month_day.day());

		const auto format_string =
#ifdef USE_STD_FORMAT
			std::format
#else
			fmt::format
#endif
			("{}_{:04d}-{:02d}-{:02d}", title_, static_cast<int>(year),
			 static_cast<unsigned>(month), static_cast<unsigned>(day));

		return { std::filesystem::path(format_string).replace_extension(".log"),
				 std::filesystem::path(format_string).replace_extension(".backup") };
	}

	auto file_job::read_lines(const std::string& file_name) -> std::vector<std::string>
	{
		if (!std::filesystem::exists(file_name))
		{
			return {};
		}

		std::ifstream readfile(file_name);
		if (!readfile.is_open())
		{
			return {};
		}

		std::string line;
		std::vector<std::string> lines;
		while (std::getline(readfile, line))
		{
			line += '\n';
			lines.push_back(std::move(line));
		}

		return lines;
	}

	auto file_job::append_lines(const std::string& file_name,
								const std::vector<std::string>& messages) -> bool
	{
		std::ofstream outfile(file_name, std::ios_base::out | std::ios_base::app);
		if (!outfile.is_open())
		{
			return false;
		}

		for (const auto& message : messages)
		{
			outfile << message;
		}

		return true;
	}
} // namespace log_module