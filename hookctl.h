#ifndef YAN_HOOKCTL_H
#define YAN_HOOKCTL_H

#include <map>

#include <curl/curl.h>

namespace yhook
{
	class state
	{
	private:
		state();
		~state();

	public:
		static state& instance();

		state(const state&) = delete;
		state(state&&) noexcept = delete;
		state& operator=(const state&) = delete;
		state& operator=(state&&) noexcept = delete;
	};

	class parser
	{
	public:
		parser();
		parser(const std::string& data);

		virtual ~parser() = default;

		std::string field(const std::string name) const;

	protected:
		void parse_data(const std::string& data) noexcept;

	private:
		void insert_field(const std::string key, const std::string value) noexcept;

		std::map<std::string, std::string> _data;
	};

	class reader : public parser
	{
	public:
		reader(CURL* curl, const std::string url);

		void insert_chunk(const char* data, const size_t size) noexcept;

	private:
		std::string _data_raw;
	};

	namespace callback
	{
		size_t write(char* data, size_t size, size_t members_num, reader* owner);
	};

	struct hook_fields
	{
		std::string post_format() const noexcept;

		std::string& operator[](const std::string name) noexcept;

		std::map<std::string, std::string> fields;

	private:
		static std::string url_encode(const std::string message) noexcept;

		static std::string encode_character(const char c) noexcept;
		static bool special_character(const char c) noexcept;
	};

	class hook
	{
	public:
		hook(const std::string url);

		~hook();

		void send(const hook_fields fields) const;

	private:
		void load_info(const std::string url);

		std::string _send_url;

		mutable char _error_buffer[CURL_ERROR_SIZE];
		mutable CURL* _curl;
	};

	class controller
	{
	public:
		controller(const std::filesystem::path webhook_path);

		void set_name(const std::string name) noexcept;

		void send(const std::string message) const;
		void send_as(const std::string message, const std::string name) const;
		void send_default(const std::string message) const;

	private:
		static std::string parse_path(const std::filesystem::path path);

		std::string _name = "webhook";

		hook _hook;
	};
}

#endif