#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>

#include "notifyctl.h"


using namespace ynotif;

parser::parser()
{
}

parser::parser(const std::string& config)
{
	parse_config(config);
}

void parser::parse_config(const std::string& config, const bool log)
{
	notifications.clear();

	if(log)
		std::cout << "lexer started" << std::endl;

	const std::vector<lexeme> lexemes = lexer(config);

	if(log)
		std::cout << "lexer finished" << std::endl;

	std::vector<notify_argument> arguments;

	std::string name;
	std::string value;

	int indent_level = 0;

	if(log)
		std::cout << "began parsing" << std::endl;

	for(const lexeme l : lexemes)
	{
		switch(l.id)
		{
			case lexeme_id::open_bracket:
				++indent_level;
			break;

			case lexeme_id::close_bracket:
				if(indent_level==2)
					arguments.push_back(parse_arg(name, value));

				if(indent_level==1)
					parse_notification(arguments);

				--indent_level;
			break;

			case lexeme_id::pair_divisor:
			break;

			case lexeme_id::key:
				if(log)
					std::cout << "variable name " << l.value << std::endl;
				name = l.value;
			break;

			case lexeme_id::value:
				if(log)
					std::cout << "variable value " << l.value << std::endl;
				value = l.value;
			break;
		}
	}

	if(log)
		std::cout << "finished parsing" << std::endl;
}

void parser::parse_notification(std::vector<notify_argument>& arguments)
{
	const notify_argument notification_arg = parse_notification_type(arguments);

	switch(notification_arg.id)
	{
		case argument_type::daily:
		{
			const std::string& time = notification_arg.value;

			const int divisor_index = time.find(':');

			const int hour = std::stoi(time.substr(0, divisor_index));
			const int minute = std::stoi(time.substr(divisor_index+1));

			notifications.push_back(std::make_unique<daily_notification>
			(hour, minute, arguments));
		}
		break;
	}

	fields_check(arguments);
}

void parser::fields_check(std::vector<notify_argument>& arguments)
{
	bool contains_message = false;

	for(const auto& arg : arguments)
	{
		switch(arg.id)
		{
			case argument_type::message:
				contains_message = true;
			break;

			default:
			break;
		}
	}

	if(!contains_message)
		throw std::runtime_error("notification does not contain a message");
}

parser::notify_argument parser::parse_notification_type(std::vector<notify_argument>& arguments)
{
	for(auto iter = arguments.begin(); iter!=arguments.end(); ++iter)
	{
		const notify_argument& arg = *iter;

		switch(arg.id)
		{
			case argument_type::daily:
			{
				notify_argument c_arg = arg;
				arguments.erase(iter);
				return c_arg;
			}

			default:
			break;
		}
	}

	throw std::runtime_error("parse_notification_type did not find a notify argument");
}

parser::notify_argument parser::parse_arg(const std::string name, const std::string value)
{
	if(name=="daily")
		return notify_argument{argument_type::daily, value};

	if(name=="name")
		return notify_argument{argument_type::name, value};

	if(name=="message")
		return notify_argument{argument_type::message, value};

	throw std::runtime_error(name+std::string("(value=")+value+std::string(") is an unknown argument type"));
}

std::vector<parser::lexeme> parser::lexer(const std::string& text) noexcept
{
	std::vector<lexeme> lexemes;

	bool ignore_next = false;

	bool double_quotes = false;
	bool reading_string = false;
	std::string c_value = "";

	for(const char& c : text)
	{
		switch(c)
		{
			case '\\':
				ignore_next = true;
			continue;

			case '{':
				if(reading_string || ignore_next)
					c_value.push_back(c);
				else
					lexemes.push_back({lexeme_id::open_bracket, ""});
			break;

			case '}':
				if(reading_string || ignore_next)
				{
					c_value.push_back(c);
				} else
				{
					if(lexemes.back().id==lexeme_id::pair_divisor)
					{
						lexemes.push_back({lexeme_id::value, c_value});
						c_value = "";
					}
					lexemes.push_back({lexeme_id::close_bracket, ""});
				}
			break;

			case ',':
				if(reading_string || ignore_next)
				{
					c_value.push_back(c);
				} else
				{
					if(lexemes.back().id==lexeme_id::open_bracket)
					{
						lexemes.push_back({lexeme_id::key, c_value});
						c_value = "";
					}
					lexemes.push_back({lexeme_id::pair_divisor, ""});
				}
			break;

			case '\'':
			case '"':
				if((reading_string && c=='\'' && double_quotes)
					|| (reading_string && c=='"' && !double_quotes)
					|| ignore_next)
				{
					c_value.push_back(c);
				} else
				{
					if(!reading_string)
					{
						if(c=='"')
							double_quotes = true;
						else
							double_quotes = false;
					}
					reading_string = !reading_string;
				}
			break;

			case ' ':
			case '\n':
				if(reading_string || ignore_next)
					c_value.push_back(c);
			break;

			default:
				c_value.push_back(c);
			break;
		}

		ignore_next = false;
	}

	return lexemes;
}

parser::daily_notification::daily_notification(int h, int m, std::vector<notify_argument> args)
: hour(h), minute(m), arguments(args)
{
}

int parser::daily_notification::seconds_next() const noexcept
{
	using namespace std::literals;

	const auto now{std::chrono::system_clock::now()};

	std::time_t now_t = std::chrono::system_clock::to_time_t(now);

	std::tm local_tm = *localtime(&now_t);

	const int c_hour = local_tm.tm_hour;
	const int c_minute = local_tm.tm_min;

	const std::chrono::hours n_hour(hour);
	const std::chrono::minutes n_minute(minute);

	const std::chrono::hours cc_hour(c_hour);
	const std::chrono::minutes cc_minute(c_minute);

	auto n_time = n_hour+n_minute;
	const auto cc_time = cc_hour+cc_minute;

	if(c_hour>hour || (c_hour==hour && c_minute>=minute))
	{
		//its tommorow
		n_time += 24h;
	}

	return std::chrono::duration_cast<std::chrono::seconds>(n_time-cc_time).count();
}

const std::vector<parser::notify_argument>& parser::daily_notification::get_arguments() const noexcept
{
	return arguments;
}

controller::controller(const std::filesystem::path config)
: _config_path(config)
{
	parse_config(parse_path(config), true);
}

void controller::start(const yhook::controller& sender, const bool log) noexcept
{
	int smallest_time = -1;
	const std::vector<notify_argument>* args;

	for(auto& notification : notifications)
	{
		const int next_time = notification->seconds_next();
		if(smallest_time>next_time || smallest_time==-1)
		{
			smallest_time = next_time;
			args = &notification->get_arguments();
		}
	}

	if(log)
		std::cout << "going to sleep, sending the notification in " << smallest_time << " seconds ^-^" << std::endl;
	send_delayed(sender, smallest_time, *args);


	const auto c_time = std::filesystem::last_write_time(_config_path);
	if(c_time!=_config_last_edit)
	{
		parse_config(parse_path(_config_path));
	}

	start(sender, log);
}

void controller::send_delayed(const yhook::controller& sender, const int delay_seconds, const std::vector<notify_argument>& args) const noexcept
{
	std::this_thread::sleep_for(std::chrono::seconds(delay_seconds));

	std::string send_name = "";
	std::string send_message = "";

	for(const auto& arg : args)
	{
		switch(arg.id)
		{
			case argument_type::name:
				send_name = arg.value;
			break;

			case argument_type::message:
				send_message = arg.value;
			break;

			default:
			break;
		}
	}

	if(send_name=="")
	{
		sender.send_default(send_message);
	} else
	{
		sender.send_as(send_message, send_name);
	}
}

std::string controller::parse_path(const std::filesystem::path path)
{
	if(!std::filesystem::exists(path))
		throw std::runtime_error(std::string("path doesnt exist: ")+path.string());

	_config_last_edit = std::filesystem::last_write_time(_config_path);

	std::ifstream config_file(path);

	std::stringstream read_string;
	read_string << config_file.rdbuf();

	return read_string.str();
}