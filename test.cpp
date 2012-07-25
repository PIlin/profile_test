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
	virtual void specMethVoidRes(int) = 0;
	virtual int specMethIntRes(double) = 0;
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

			// open channel, whatever
		}

		virtual void specificMethod() 
		{
			std::cout << __PRETTY_FUNCTION__ << std::endl;
		}

		virtual void specMethVoidRes(int a)
		{
			std::cout << __PRETTY_FUNCTION__ << " a = " << a << std::endl;
		}

		virtual int specMethIntRes(double b)
		{
			std::cout << __PRETTY_FUNCTION__ << " b = " << b << std::endl;
			return (int)b;
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
		if (profile)
			return profile->getApi();
		else
			return 0;
	}

	void releaseProfile()
	{
		std::cout << __PRETTY_FUNCTION__ << std::endl;
	}

	void killProfile()
	{
		delete profile;
		profile = 0;
	}
};

// end of profile manager

// handler

#define HANDLER_BOILERPLATE_CALL(API, NAME, RESULT, ...) 		\
	do { 														\
		BaseApi* ba = pm.getProfile();							\
		if (ba)	{												\
			RESULT = static_cast<API*>(ba)->NAME(__VA_ARGS__);	\
			pm.releaseProfile();								\
			return true;										\
		}														\
		return false;											\
	} while (0)

#define HANDLER_BOILERPLATE_VOID_CALL(API, NAME, ...)			\
	do { 														\
		BaseApi* ba = pm.getProfile();							\
		if (ba)	{												\
			static_cast<API*>(ba)->NAME(__VA_ARGS__);			\
			pm.releaseProfile();								\
			return true;										\
		}														\
		return false;											\
	} while (0)

struct SpecificHandler
{
	PM& pm;
	SpecificHandler(PM& pm) : pm(pm) {}

	bool specificMethod() 
	{ 

		BaseApi* ba = pm.getProfile();
		if (ba)	{
			static_cast<SpecificApi*>(ba)->specificMethod();
			pm.releaseProfile();
			return true;
		}
		return false;	
	}

	bool specMethVoidRes(int const a)
	{
		HANDLER_BOILERPLATE_VOID_CALL(SpecificApi, specMethVoidRes, a);
	}

	bool specMethIntRes(double const b, int & res)
	{
		HANDLER_BOILERPLATE_CALL(SpecificApi, specMethIntRes, res, b);
	}
};

int main()
{
	PM pm;
	SpecificHandler h(pm);
	std::cout << h.specificMethod() << std::endl;
	int res = 0;
	std::cout << h.specMethIntRes(100.345, res) << " res = " << res << std::endl;
	pm.killProfile();
	std::cout << h.specMethVoidRes(100) << std::endl;
}

