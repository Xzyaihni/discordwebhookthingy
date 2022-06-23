#ifndef YAN_NOTIFY_H
#define YAN_NOTIFY_H

#include <filesystem>
#include <vector>

#include "hookctl.h"

namespace ynotif
{
	class parser
	{
	private:
		enum class lexeme_id {open_bracket, close_bracket, pair_divisor, key, value};

		struct lexeme
		{
			lexeme_id id;
			std::string value;
		};

	protected:
		enum class argument_type {daily, message, name};

		struct notify_argument
		{
			argument_type id;
			std::string value;
		};

		struct notification
		{
			virtual ~notification() = default;

			virtual int seconds_next() const noexcept = 0;
			virtual const std::vector<notify_argument>& get_arguments() const noexcept = 0;
		};

		struct daily_notification : public notification
		{
			daily_notification(int h, int m, std::vector<notify_argument> args);

			int seconds_next() const noexcept override;
			const std::vector<notify_argument>& get_arguments() const noexcept override;

			int hour;
			int minute;
			std::vector<notify_argument> arguments;
		};

	public:
		parser();
		parser(const std::string& config);
		virtual ~parser() = default;

	protected:
		void parse_config(const std::string& config, const bool log = false);

		std::vector<std::unique_ptr<notification>> notifications;

	private:
		void parse_notification(std::vector<notify_argument>& arguments);
		static void fields_check(std::vector<notify_argument>& arguments);
		static notify_argument parse_notification_type(std::vector<notify_argument>& arguments);
		static notify_argument parse_arg(const std::string name, const std::string value);

		static std::vector<lexeme> lexer(const std::string& text) noexcept;
	};

	class controller : protected parser
	{
	public:
		controller(const std::filesystem::path config);

		void start(const yhook::controller& sender, const bool log = false) noexcept;

	private:
		void send_delayed(const yhook::controller& sender, const int delay_seconds, const std::vector<notify_argument>& args) const noexcept;

		std::string parse_path(const std::filesystem::path path);

		std::filesystem::path _config_path;
		std::string _config_data;
		std::filesystem::file_time_type _config_last_edit;
	};
};

#endif