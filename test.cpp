#include <iostream>

// base_profile.hpp

struct BaseApi;

struct BaseProfile
{
	virtual BaseApi* getApi() = 0;
protected:
	virtual ~BaseProfile() {}
};

// end base_profile.hpp

// profile.hpp

struct Profile : public virtual BaseProfile
{
	int getInstId() const { return inst_id; }
	int getServId() const { return serv_id; }
	int getError() const { return error; }
	bool isEnabled() const { return enabled; }

protected:

	virtual void onEnabledChange(bool new_state) = 0;

	void setError(int err) { error = err; }

	Profile(int inst_id, int serv_id) :
		inst_id(inst_id),
		serv_id(serv_id),
		error(0),
		enabled(false)
	{}

private:

	void setEnabled(bool state)
	{
		if (state != enabled)
		{
			enabled = state;
			onEnabledChange(enabled);
		}
	}

	int inst_id;
	int serv_id;
	int error;
	bool enabled;

	friend struct PM;
};

// end of profile.hpp

// base_api.hpp

struct BaseApi : public virtual BaseProfile
{
protected:
	virtual ~BaseApi() {}
};

// specific_api.hpp

struct SpecificApi : public BaseApi
{
	virtual void specificMethod() = 0;
protected:
	virtual ~SpecificApi() {}
};

// end of specific_api.hpp

// end of base_api.hpp

// profile_lib.hpp

struct ProfileLibApi
{
	Profile* (*createProfile)(int, int);
	char const* (*getProfileName)();
	int (*getProfileVersion)();
};
#define LIBRARY_BOILERPLATE(PROF, NAME, VERSION) 															\
	Profile* createProfile(int instance_id, int service_id) { return new PROF(instance_id, service_id); } 	\
	char const* getProfileName() { return NAME; }															\
	int getProfileVersion() { return VERSION; }																\
	static ProfileLibApi PROFILE_LIB_API_TABLE = { createProfile, getProfileName, getProfileVersion };


#define PROFILE_BOILERPLATE(PROF)												\
	PROF(int inst_id, int serv_id) : Profile(inst_id, serv_id) {}				\
	virtual BaseApi* getApi() { return this; }

// end of profile_lib.hpp

// specific_profile.hpp

namespace LibraryCode
{
	struct SpecificProfile : public SpecificApi, public Profile
	{
		PROFILE_BOILERPLATE(SpecificProfile)

		virtual void onEnabledChange(bool new_state) 
		{
			std::cout << __PRETTY_FUNCTION__ << " new state = " << new_state << std::endl;
		}

		virtual void specificMethod() 
		{
			std::cout << __PRETTY_FUNCTION__ << std::endl;
		}

	};

	LIBRARY_BOILERPLATE(SpecificProfile, "SpecificProfile", 9000)
}

// end of specific_profile.hpp

// profile manager

void* pseudo_dlsym()
{
	return &LibraryCode::PROFILE_LIB_API_TABLE;
}


struct PM
{
	Profile* profile;
	PM()
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
		ProfileLibApi* plapi = reinterpret_cast<ProfileLibApi*>(pseudo_dlsym());
		std::cout << "loading profile" << std::endl;
		std::cout << "name:    " << plapi->getProfileName() << std::endl;
		std::cout << "version: " << plapi->getProfileVersion() << std::endl;
		profile = plapi->createProfile(28, 42);
		profile->setEnabled(true);
	}

	~PM()
	{
		killProfile();
	}

	BaseApi* getProfile()
	{
		return profile->getApi();
	}

	void killProfile()
	{
		delete profile;
		profile = 0;
	}
};

// end of profile manager

// handler

struct Handler
{
	PM& pm;
	Handler(PM& pm) : pm(pm) {}
	void specificMethod() 
	{ 
		BaseApi* ba = pm.getProfile();
		if (ba)
		{
			SpecificApi* api = static_cast<SpecificApi*>(ba);
			return api->specificMethod();	
		}
	}
};

int main()
{
	PM pm;
	Handler h(pm);
	h.specificMethod();
	pm.killProfile();
}

