#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "hookctl.h"

using namespace yhook;


state::state()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

state::~state()
{
	curl_global_cleanup();
}

state& state::instance()
{
	static state g_instance;
	return g_instance;
}


size_t callback::write(char* data, size_t size, size_t members_num, reader* owner)
{
	const size_t read_size = size * members_num;

	owner->insert_chunk(data, read_size);

	return read_size;
}


parser::parser()
{
}

parser::parser(const std::string& data)
{
	parse_data(data);
}

std::string parser::field(const std::string name) const
{
	return _data.at(name);
}

void parser::parse_data(const std::string& data) noexcept
{
	std::string s_key = "";
	std::string s_value = "";

	bool currently_string = false;

	bool dict_value = false;

	//skip first and last characters because they're {}
	for(int i = 1; i < data.size()-1; ++i)
	{
		const char& c = data[i];

		if(c=='"')
		{
			currently_string = !currently_string;
			continue;
		}

		if(c==':')
		{
			dict_value = true;
			continue;
		}

		if(c==',')
		{
			dict_value = false;

			insert_field(s_key, s_value);

			s_key = "";
			s_value = "";

			continue;
		}

		if(c==' ' && !currently_string)
			continue;

		if(!dict_value)
		{
			s_key.push_back(c);
		} else
		{
			s_value.push_back(c);
		}
	}

	insert_field(s_key, s_value);
}

void parser::insert_field(const std::string key, const std::string value) noexcept
{
	_data[key] = value;
}


reader::reader(CURL* curl, const std::string url)
{
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback::write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

	const CURLcode res = curl_easy_perform(curl);
	if(res != CURLE_OK)
		throw std::runtime_error(curl_easy_strerror(res));

	parse_data(_data_raw);
}

void reader::insert_chunk(const char* data, const size_t size) noexcept
{
	_data_raw.insert(_data_raw.size(), data, size);
}

std::string hook_fields::post_format() const noexcept
{
	std::string post_fields = "";

	for(const auto& [key, value] : fields)
	{
		post_fields += url_encode(key) + '=' + url_encode(value) + '&';
	}
	post_fields.pop_back();

	return post_fields;
}

std::string& hook_fields::operator[](const std::string name) noexcept
{
	return fields[name];
}

std::string hook_fields::url_encode(const std::string message) noexcept
{
	std::string encoded_string = "";
	for(const char& c : message)
	{
		if(!special_character(c))
			encoded_string += c;
		else
			encoded_string += encode_character(c);
	}
	return encoded_string;
}

std::string hook_fields::encode_character(const char c) noexcept
{
	switch(c)
	{
		case '!':
		return "%21";

		case '#':
		return "%23";

		case '$':
		return "%24";

		case '&':
		return "%26";

		case '\'':
		return "%27";

		case '(':
		return "%28";

		case ')':
		return "%29";

		case '*':
		return "%2A";

		case '+':
		return "%2B";

		case ',':
		return "%2C";

		case '/':
		return "%2F";

		case ':':
		return "%3A";

		case ';':
		return "%3B";

		case '=':
		return "%3D";

		case '?':
		return "%3F";

		case '@':
		return "%40";

		case '[':
		return "%5B";

		case ']':
		return "%5D";

		default:
		return std::string(1, c);
	}
}

bool hook_fields::special_character(const char c) noexcept
{
	return c<94 && (c<47 || c>57);
}

hook::hook(const std::string url)
{
	state::instance();
	_curl = curl_easy_init();

	if(_curl==NULL)
		throw std::runtime_error("failed to initialize CURL");

	load_info(url);
}

hook::~hook()
{
	curl_easy_cleanup(_curl);
}

void hook::send(const hook_fields fields) const
{
	const std::string c_fields = fields.post_format();

	#ifdef DEBUG
		curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);
	#endif

	curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error_buffer);
	curl_easy_setopt(_curl, CURLOPT_URL, _send_url.c_str());
	curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, c_fields.c_str());

	const CURLcode res = curl_easy_perform(_curl);
	if(res != CURLE_OK)
		throw std::runtime_error(curl_easy_strerror(res)+std::string(" (")+std::string(_error_buffer)+std::string(")"));
}

void hook::load_info(const std::string url)
{
	reader c_data(_curl, url);

	_send_url = "https://discord.com/api/webhooks/";
	_send_url += c_data.field("id");
	_send_url += "/";
	_send_url += c_data.field("token");
}

controller::controller(const std::filesystem::path webhook_path)
: _hook(parse_path(webhook_path))
{
}

void controller::set_name(const std::string name) noexcept
{
	_name = name;
}

void controller::send(const std::string message) const
{
	send_as(message, _name);
}

void controller::send_as(const std::string message, const std::string name) const
{
	hook_fields fields;
	fields["content"] = message;
	fields["username"] = name;

	_hook.send(std::move(fields));
}

void controller::send_default(const std::string message) const
{
	hook_fields fields;
	fields["content"] = message;

	_hook.send(std::move(fields));
}

std::string controller::parse_path(const std::filesystem::path path)
{
	if(!std::filesystem::exists(path))
		throw std::runtime_error(std::string("path doesnt exist: ")+path.string());

	std::ifstream url_file(path);

	std::stringstream read_string;
	read_string << url_file.rdbuf();

	return read_string.str();
}