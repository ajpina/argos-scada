#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char  _DATE[] = "02";
	static const char  _MONTH[] = "08";
	static const char  _YEAR[] = "2010";
	static const char  _UBUNTU_VERSION_STYLE[] = "10.08";
	
	//Software Status
	static const char  _STATUS[] = "Alpha";
	static const char  _STATUS_SHORT[] = "a";
	
	//Standard Version Type
	static const long  _MAJOR = 0;
	static const long  _MINOR = 8;
	static const long  _BUILD = 740;
	static const long  _REVISION = 3956;
	
	//Miscellaneous Version Types
	static const long  _BUILDS_COUNT = 892;
	#define  _RC_FILEVERSION 0,8,740,3956
	#define  _RC_FILEVERSION_STRING "0, 8, 740, 3956\0"
	static const char  _FULLVERSION_STRING[] = "0.8.740.3956";
	
	//SVN Version
	static const char  _SVN_REVISION[] = "175";
	static const char  _SVN_DATE[] = "2009-04-01T15:48:17.447415Z";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long  _BUILD_HISTORY = 40;
	

}
#endif //VERSION_H
