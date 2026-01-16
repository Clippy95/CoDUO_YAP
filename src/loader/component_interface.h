#pragma once
#include <string>

class component_interface
{
public:
	virtual ~component_interface()
	{
	}

	virtual void post_start()
	{
	}

	virtual void post_load()
	{
	}

	virtual void pre_destroy()
	{
	}

	virtual void post_unpack()
	{
	}

	virtual void post_game_sp()
	{
	}

	virtual void post_cgame()
	{
	}
	virtual void post_ui()
	{
	}

	virtual void on_ogl_load(HMODULE tOHGL)
	{
	}

	virtual void* load_import([[maybe_unused]] const std::string& library, [[maybe_unused]] const std::string& function)
	{
		return nullptr;
	}

	virtual bool is_supported()
	{
		return true;
	}
};