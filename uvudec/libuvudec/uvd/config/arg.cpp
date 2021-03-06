/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

/*
FIXME: naked arg stuff added a number of quick hacks and we should rewrite using polymorphism or something
*/

#include "uvd/config/arg.h"
#include "uvd/config/arg_property.h"
#include "uvd/config/arg_util.h"
#include "uvd/config/config.h"
#include "uvd/language/language.h"
#include "uvd/util/debug.h"
#include "uvd/util/types.h"
#include "uvd/util/util.h"
#include "uvd/util/version.h"
#include "uvd/core/analysis.h"
#include "uvd/plugin/engine.h"
#include <vector>

static uv_err_t argParser(const UVDArgConfig *argConfig, std::vector<std::string> argumentArguments, void *user);

//#undef printf_args_debug
//#define printf_args_debug printf

/*
UVDArgConfigs
*/

UVDArgConfigs::UVDArgConfigs()
{
}

UVDArgConfigs::~UVDArgConfigs()
{
	for( ArgConfigs::iterator iter = m_argConfigs.begin(); iter != m_argConfigs.end(); ++iter )
	{
		delete (*iter).second;
	}
	m_argConfigs.clear();
}

uv_err_t UVDArgConfigs::registerDefaultArgument(UVDArgConfigHandler handler,
		const std::string &helpMessage,
		uint32_t minRequired,
		bool combine,
		bool alwaysCall,
		void *user)
{
	UVDArgConfig *argConfig = NULL;

	argConfig = new UVDArgConfig(handler, helpMessage, minRequired, combine, alwaysCall, user);
	uv_assert_ret(argConfig);
	m_argConfigs[""] = argConfig;

	return UV_ERR_OK;
}

uv_err_t UVDArgConfigs::registerArgument(const std::string &propertyForm,
		char shortForm, std::string longForm, 
		std::string helpMessage,
		std::string helpMessageExtra,
		uint32_t numberExpectedValues,
		UVDArgConfigHandler handler,
		bool hasDefault,
		void *user)
{
	UVDArgConfig *argConfig = NULL;
	
	argConfig = new UVDArgConfig(propertyForm, shortForm, longForm, helpMessage, helpMessageExtra, numberExpectedValues, handler, hasDefault, user);
	uv_assert_ret(argConfig);
	m_argConfigs[propertyForm] = argConfig;

	return UV_ERR_OK;
}

/*
UVDArgRegistry
*/

UVDArgRegistry::UVDArgRegistry()
{
}

UVDArgRegistry::UVDArgRegistry(UVDArgConfigs *argConfigs)
{
	m_argConfigsSet.insert(argConfigs);
}

UVDArgRegistry::~UVDArgRegistry()
{
}

uv_err_t UVDArgRegistry::newArgConfgs(UVDArgConfigs **out)
{
	UVDArgConfigs *ret = NULL;

	ret = new UVDArgConfigs();
	uv_assert_ret(ret);
	m_argConfigsSet.insert(ret);
	if( out )
	{
		*out = ret;
	}
	return UV_ERR_OK;
}

uv_err_t UVDArgRegistry::printUsage(const std::string &indent)
{
	for( std::set<UVDArgConfigs *>::iterator iter = m_argConfigsSet.begin();
			iter != m_argConfigsSet.end(); ++iter )
	{
		UVDArgConfigs *argConfigs = *iter;

		uv_assert_err_ret(argConfigs->printUsage(indent));
	}
	
	return UV_ERR_OK;
}

uv_err_t UVDArgConfigs::printUsage(const std::string &indent)
{
	for( ArgConfigs::iterator iter = m_argConfigs.begin();
			iter != m_argConfigs.end(); ++iter )
	{
		UVDArgConfig *argConfig = (*iter).second;
		
		uv_assert_ret(argConfig);		
		uv_assert_err_ret(argConfig->print(indent));
	}
	return UV_ERR_OK;
}

uv_err_t UVDArgRegistry::processMain(int argc, char *const *argv)
{
	//Strings by themeslves
	std::vector<std::string> argsIn;
	//Strings tagged as from the command line
	UVDRawArgs args;
	
	argsIn = charPtrArrayToVector(argv, argc);
	uv_assert_err_ret(UVDCommandLineArg::fromStringVector(argsIn, args));
	
	uv_assert_ret(m_argConfigsSet.size() == 1);
	
	for( std::set<UVDArgConfigs *>::iterator iter = m_argConfigsSet.begin(); iter != m_argConfigsSet.end(); ++iter )
	{
		UVDArgConfigs *argConfigs = *iter;
		uv_err_t rc = UV_ERR_GENERAL;

		uv_assert_ret(argConfigs);

		//FIXME: this will only work for one arg set b/c of urecognized args
		//We need to parse args and dispatch to inivdidual parsers to preserve order
		rc = UVDArgConfig::process(*argConfigs, args,
				true, NULL);
		uv_assert_err_ret(rc);
		printf_args_debug("UVDArgConfig::process() rc: %d\n", rc);
		if( rc == UV_ERR_DONE )
		{
			return UV_ERR_DONE;
		}
		else
		{
			return UV_DEBUG(rc);
		}
	}
	uv_assert_ret(args.m_args.empty());
	
	return UV_DEBUG(UV_ERR_GENERAL);
}

/*
uv_err_t UVDArgRegistry::processStringVectorWithRemove(std::vector<std::string> &args)
{
	return UV_DEBUG(UV_ERR_GENERAL);
}
*/

/*
UVDArgConfig
*/

UVDArgConfig::UVDArgConfig()
{
	m_hasDefault = false;
	m_numberExpectedValues = 0;
	m_shortForm = 0;
}

UVDArgConfig::UVDArgConfig(UVDArgConfigHandler handler,
		const std::string &helpMessage,
		uint32_t minRequired,
		bool combine,
		bool alwaysCall,
		void *user)
{
	//Union full zero
	m_shortForm = 0;

	m_combine = combine;
	m_handler = handler;
	m_alwaysCall = alwaysCall;
	m_numberExpectedValues = minRequired;
	m_helpMessage = helpMessage;
	m_user = user;
}
		
UVDArgConfig::UVDArgConfig(const std::string &propertyForm,
		char shortForm, std::string longForm, 
		std::string helpMessage,
		std::string helpMessageExtra,
		uint32_t numberExpectedValues,
		UVDArgConfigHandler handler,
		bool hasDefault,
		void *user)
{
	m_propertyForm = propertyForm;
	m_shortForm = shortForm;
	m_longForm = longForm;
	m_helpMessage = helpMessage;
	m_helpMessageExtra = helpMessageExtra;
	m_numberExpectedValues = numberExpectedValues;
	m_handler = handler;
	m_hasDefault = hasDefault;
	m_user = user;
}

UVDArgConfig::~UVDArgConfig()
{
}

bool UVDArgConfig::isNakedHandler() const
{
	return m_propertyForm == "";
}

/*
bool UVDArgConfig::operator==(const std::string &r) const
{
	return m_propertyForm == r;
}
*/

uv_err_t UVDArgConfig::process(const UVDArgConfigs &argConfigs, UVDRawArgs &args,
		bool printErrors, UVDArgConfigs *ignoredArgs)
{
	//Used if m_alwaysCall or m_combine is used
	std::vector<std::string> nakedArgs;
	const UVDArgConfig *nakedConfig = NULL;

	for( UVDArgConfigs::ArgConfigs::const_iterator iter = argConfigs.m_argConfigs.begin();
			iter != argConfigs.m_argConfigs.end(); ++iter )
	{
		const UVDArgConfig *argConfig = (*iter).second;
			
		uv_assert_ret(argConfig);
		if( argConfig->isNakedHandler() )
		{
			nakedConfig = argConfig;
			break;
		}
	}

	/*
	Core high level argument dispatching
	We could store args as property map, but we'd still have to iterate over to match each part
	*/
	//Iterate for each actual command line argument
	//Note that args should NOT have prog name
	for( std::vector<std::string>::size_type argsIndex = 0; argsIndex < args.m_args.size(); ++argsIndex )
	{
		UVDRawArg *argObject = args.m_args[argsIndex];
		std::string arg = argObject->m_token;
		std::vector<UVDParsedArg> parsedArgs;

		printf_args_debug("arg: %s\n", arg.c_str());
		uv_assert_err_ret(UVDProcessArg(arg, parsedArgs));

		//Now iterate for each logical command line argument (such as from -abc)
		for( std::vector<UVDParsedArg>::iterator iter = parsedArgs.begin(); iter != parsedArgs.end(); ++iter )
		{
			bool ignoreEarlyArg = false;
			UVDParsedArg &parsedArg = *iter;
			const UVDArgConfig *matchedConfig = NULL;
			uv_err_t matchRc = UV_ERR_GENERAL;
			
			//Extract the argument info for who should handle it
			matchRc = UVDMatchArgConfig(argConfigs, parsedArg, &matchedConfig);
			printf_args_debug("matchRc: %d\n", matchRc);
			//Make sure to return UV_ERR_NOTFOUND if generated
			if( UV_FAILED(matchRc) )
			{
				//FIXME: hack
				//don't error if it was an early initialization arg
				if( ignoredArgs && UV_SUCCEEDED(UVDMatchArgConfig(*ignoredArgs, parsedArg, &matchedConfig)) )
				{
					ignoreEarlyArg = true;
				}
				else
				{
					if( printErrors )
					{
						printf_error("unknown argument: %s\n", parsedArg.m_raw.c_str());
						return UV_DEBUG(matchRc);
					}
					else
					{
						//We should be IN the early config
						//Ignore this argument
						continue;
					}
				}
			}
			uv_assert_ret(matchedConfig);
			std::vector<std::string> argumentArguments;
			//For now hand this off as single args
			//Consider instead grouping into one large list
			if( parsedArg.m_keyForm == UVD_ARG_FORM_NAKED )
			{
				//We should be during the primary parse
				if( ignoreEarlyArg )
				{
					printf_error("undecorated argument was matched to early config: <%s>\n", parsedArg.m_raw.c_str());
					return UV_DEBUG(UV_ERR_GENERAL);
				}
				uv_assert_ret(nakedConfig == matchedConfig);
				nakedArgs.push_back(parsedArg.m_key);
				if( nakedConfig->m_combine )
				{
					//Don't continue to process this arg now
					continue;
				}
				argumentArguments.push_back(parsedArg.m_key);
			}
			//Do we need to consume an arg for this?
			else if( matchedConfig->m_numberExpectedValues > 0 )
			{
				uv_assert_ret(matchedConfig->m_numberExpectedValues == 1);
				if( parsedArg.m_embeddedValPresent )
				{
					argumentArguments.push_back(parsedArg.m_embeddedVal);
				}
				else
				{
					//Only grab the next argument if we don't have a default value
					if( !matchedConfig->m_hasDefault )
					{
						//we need to grab next then
						++argsIndex;
						uv_assert_ret(argsIndex < args.m_args.size());
						argObject = args.m_args[argsIndex];
						argumentArguments.push_back(argObject->m_token);
					}
				}
			}
			//And call their handler
			if( !ignoreEarlyArg )
			{
				uv_err_t handlerRc = matchedConfig->m_handler(matchedConfig, argumentArguments, matchedConfig->m_user);
				printf_args_debug("handler RC: %d\n", handlerRc);
				if( UV_FAILED(handlerRc) )
				{
					printf_args_debug("handler returned error processing argument: %s\n", parsedArg.m_raw.c_str());
					return UV_DEBUG(UV_ERR_GENERAL);
				}
				
				//Mark it as parsed
				//NOTE: unit test and other stand alone apps don't use g_config
				if( g_config ) {
					/*
					if( g_config->m_argEngine.m_lastSource.find(matchedConfig->m_propertyForm) == g_config->m_argEngine.m_lastSource.end() ) {
						printf_err("Couldn't match up: %s\n", matchedConfig->m_propertyForm.c_str());
					}
					uv_assert_ret(g_config->m_argEngine.m_lastSource.find(matchedConfig->m_propertyForm) != g_config->m_argEngine.m_lastSource.end());
					*/
					g_config->m_argEngine.m_lastSource[matchedConfig->m_propertyForm] = argObject->getSource();
					//Some option like help() has been called that means we should abort program
				}
				if( handlerRc == UV_ERR_DONE )
				{
					printf_args_debug("ret happy done\n");
					return UV_ERR_DONE;
				}
			}
		}
	}
	
	//Did we get enough values?
	if( nakedConfig )
	{
		//Did we get enough args?
		if( nakedArgs.size() < nakedConfig->m_numberExpectedValues )
		{
			printf_error("need at least %d arguments, got %d\n", nakedConfig->m_numberExpectedValues, nakedArgs.size());
			return UV_DEBUG(UV_ERR_GENERAL);
		}
		//We would have already called the handler if we had previous args
		if( nakedConfig->m_combine || (nakedConfig->m_alwaysCall && nakedArgs.empty()) )
		{
			//And call their handler
			uv_err_t handlerRc = nakedConfig->m_handler(nakedConfig, nakedArgs, nakedConfig->m_user);
			uv_assert_err_ret(handlerRc);
			//Some option like help() has been called that means we should abort program
			if( handlerRc == UV_ERR_DONE )
			{
				return UV_ERR_DONE;
			}
		}
	}
	
	printf_args_debug("end return ok\n");
	return UV_ERR_OK;
}

uv_err_t UVDConfig::initArgConfig()
{
	//Now add our arguments
	
	//Actions
	uv_assert_err_ret(registerArgument(UVD_PROP_ACTION_HELP, 'h', "help", "print this message and exit", 0, argParser, false));
	uv_assert_err_ret(registerArgument(UVD_PROP_ACTION_VERSION, 0, "version", "print version and exit", 0, argParser, false));
	
	//Analysis target related
	uv_assert_err_ret(registerArgument(UVD_PROP_TARGET_ADDRESS_INCLUDE, 0, "addr-include", "inclusion address range (, or - separated)", 1, argParser, false));
	uv_assert_err_ret(registerArgument(UVD_PROP_TARGET_ADDRESS_EXCLUDE, 0, "addr-exclude", "exclusion address range (, or - separated)", 1, argParser, false));
	uv_assert_err_ret(registerArgument(UVD_PROP_TARGET_ADDRESS, 0, "analysis-address", "only output analysis data for specified address", 1, argParser, false));

	//Analysis
	uv_assert_err_ret(registerArgument(UVD_PROP_ANALYSIS_ONLY, 0, "analysis-only", "only do analysis, don't print data", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_ANALYSIS_FLOW_TECHNIQUE, 0, "flow-analysis",
			"how to determine next instruction to analyze",
				"\tlinear (linear sweep): start at beginning, read all instructions linearly, then find jump/calls (default)\n"
				"\ttrace (recursive descent): start at all vectors, analyze all segments called/branched recursivly\n"
				,	
			1, argParser, false));

	//Output
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_OPCODE_USAGE, 0, "opcode-usage", "opcode usage count table", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_JUMPED_ADDRESSES, 0, "print-jumped-addresses", "whether to print information about jumped to addresses (*1)", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_CALLED_ADDRESSES, 0, "print-called-addresses", "whether to print information about called to addresses (*1)", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_ADDRESS_COMMENT, 0, "addr-comment", "put comments on addresses", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_ADDRESS_LABEL, 0, "addr-label", "label addresses for jumping", 1, argParser, true));
	uv_assert_err_ret(registerArgument(UVD_PROP_OUTPUT_STRING_TABLE, 0, "string-table", "print string table in output", 1, argParser, true));

	return UV_ERR_OK;	
}

uv_err_t argParser(const UVDArgConfig *argConfig, std::vector<std::string> argumentArguments, void *user)
{
	UVDConfig *config = NULL;
	//If present
	std::string firstArg;
	uint32_t firstArgNum = 0;
	bool firstArgBool = true;
	
	config = (UVDConfig *)user;
	uv_assert_ret(config);
	uv_assert_ret(config->m_argv);

	uv_assert_ret(argConfig);

	if( !argumentArguments.empty() )
	{
		firstArg = argumentArguments[0];
		firstArgNum = strtol(firstArg.c_str(), NULL, 0);
		firstArgBool = UVDArgToBool(firstArg);
	}

	/*
	Actions
	*/
	if( argConfig->m_propertyForm == UVD_PROP_ACTION_HELP )
	{
		config->printHelp();
		return UV_ERR_DONE;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_ACTION_VERSION )
	{
		config->printVersion();
		return UV_ERR_DONE;
	}
	/*
	Analysis target specific
	As in, could be invalid depending on what our actual data was
	*/
	/*
	we need to parse two args at once, otherwise this is messy
	think was doing this before as comma seperate list?
	*/
	//Positive strategy: specify the address we want
	else if( argConfig->m_propertyForm == UVD_PROP_TARGET_ADDRESS_INCLUDE )
	{
		uint32_t low = 0;
		uint32_t high = 0;
		
		uv_assert_ret(!argumentArguments.empty());
		uv_assert_err_ret(parseNumericRangeString(firstArg, &low, &high));
		uv_assert_err_ret(config->addAddressInclusion(low, high));
	}
	//Negative strategy: specify the addresses we don't want
	else if( argConfig->m_propertyForm == UVD_PROP_TARGET_ADDRESS_EXCLUDE )
	{
		uint32_t low = 0;
		uint32_t high = 0;
		
		uv_assert_ret(!argumentArguments.empty());
		uv_assert_err_ret(parseNumericRangeString(firstArg, &low, &high));
		uv_assert_err_ret(config->addAddressExclusion(low, high));
	}
	else if( argConfig->m_propertyForm == UVD_PROP_TARGET_ADDRESS )
	{
		uv_assert_ret(!argumentArguments.empty());
		config->m_analysisOutputAddresses.insert(firstArgNum);
	}
	/*
	General analysis
	*/
	else if( argConfig->m_propertyForm == UVD_PROP_ANALYSIS_ONLY )
	{
		if( argumentArguments.empty() )
		{
			config->m_analysisOnly = true;
		}
		else
		{
			config->m_analysisOnly = UVDArgToBool(firstArg);
		}
	}
	else if( argConfig->m_propertyForm == UVD_PROP_ANALYSIS_FLOW_TECHNIQUE )
	{
		std::string arg = firstArg;
		if( arg == "linear" )
		{
			config->m_flowAnalysisTechnique = UVD__FLOW_ANALYSIS__LINEAR;
		}
		else if( arg == "trace" )
		{
			config->m_flowAnalysisTechnique = UVD__FLOW_ANALYSIS__TRACE;
		}
		else
		{
			printf_error("unknown flow analysis type: %s\n", arg.c_str());
			config->printHelp();
			return UV_DEBUG(UV_ERR_GENERAL);
		}
	}
	/*
	Output
	*/
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_OPCODE_USAGE )
	{
		config->m_printUsed = firstArgBool;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_JUMPED_ADDRESSES )
	{
		config->m_jumpedSources = firstArgBool;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_CALLED_ADDRESSES )
	{
		config->m_calledSources = firstArgBool;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_ADDRESS_COMMENT )
	{
		config->m_addressComment = firstArgBool;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_ADDRESS_LABEL )
	{
		config->m_addressLabel = firstArgBool;
	}
	else if( argConfig->m_propertyForm == UVD_PROP_OUTPUT_STRING_TABLE )
	{
		config->m_print_string_table = firstArgBool;
	}
	//Maybe it was in the early config?
	else
	{
		printf_error("Property not recognized in callback: %s\n", argConfig->m_propertyForm.c_str());
		return UV_DEBUG(UV_ERR_GENERAL);
	}

	return UV_ERR_OK;
}

void UVDConfig::printVersion()
{
	if( versionPrintPrefixThunk )
	{
		versionPrintPrefixThunk();
	}
	UVDPrintVersion();
}

void UVDPrintVersion()
{
	printf_help("libuvudec version %s\n", UVDGetVersion());
	printf_help("Copyright 2009-2010 John McMaster <JohnDMcMaster@gmail.com>\n");
}

uv_err_t UVDConfig::printLoadedPlugins()
{
	UVDPluginEngine *pluginEngine = &m_plugin.m_pluginEngine;

	printf_help("Loaded plugins (%d / %d):\n", pluginEngine->m_loadedPlugins.size(), pluginEngine->m_plugins.size());

	/*
	//We print args, so this list is somewhat useless, maybe as a single liner?
	for( std::map<std::string, UVDPlugin *>::iterator iter = pluginEngine->m_loadedPlugins.begin(); iter != pluginEngine->m_loadedPlugins.end(); ++iter )
	{
		UVDPlugin *plugin = (*iter).second;
		std::string name;
	
		uv_assert_ret(plugin);
		uv_assert_err_ret(plugin->getName(name));
		printf_args_debug("\t%s\n", name.c_str());
	}
	*/

	return UV_ERR_OK;
}

uv_err_t UVDArgConfig::print(const std::string &indent) const
{
	//Naked arguments don't have -- stuff
	if( !isNakedHandler() )
	{
		printf_help("%s--%s (%s): %s\n",
				indent.c_str(), m_longForm.c_str(), m_propertyForm.c_str(),
				m_helpMessage.c_str());
	}
	if( !m_helpMessageExtra.empty() )
	{
		printf_help("%s%s", indent.c_str(), m_helpMessageExtra.c_str());
	}
	return UV_ERR_OK;
}

uv_err_t UVDConfig::printUsage()
{
	const char *program_name = "";
	UVDPluginEngine *pluginEngine = NULL;
	std::string argsExtra;
	
	pluginEngine = &m_plugin.m_pluginEngine;
	
	for( UVDArgConfigs::ArgConfigs::iterator iter = m_configArgs.m_argConfigs.begin();
			iter != m_configArgs.m_argConfigs.end(); ++iter )
	{
		UVDArgConfig *argConfig = (*iter).second;
		
		uv_assert_ret(argConfig);
		if( argConfig->isNakedHandler() )
		{
			argsExtra = argConfig->m_helpMessage;
		}
	}

	if( m_argv )
	{
		program_name = m_argv[0];
	}

	printf_help("\n");
	printf_help("Usage: %s <args>%s\n", program_name, argsExtra.c_str());
	printf_help("Args:\n");
	//Maybe standard help should alphabatize these by -- form?
	//Detailed print should do by property since not all might have --
	for( UVDArgConfigs::ArgConfigs::iterator iter = m_configArgs.m_argConfigs.begin();
			iter != m_configArgs.m_argConfigs.end(); ++iter )
	{
		UVDArgConfig *argConfig = (*iter).second;
		
		uv_assert_ret(argConfig);
		
		//Print main config first
		if( pluginEngine->m_pluginArgMap.find(argConfig) == pluginEngine->m_pluginArgMap.end() )
		{
			uv_assert_err_ret(argConfig->print( ""));
		}
	}
	
	//Print special argument handling last
	printf_help("Pre plugin load args:\n");
	uv_assert_err_ret(m_plugin.m_earlyConfigArgs.printUsage());
	
	printf_help("\n");
	printLoadedPlugins();

	//Now do it by plugin type
	//this isn't terribly efficient, but who cares we should only have a handful of plugins
	for( std::map<std::string, UVDPlugin *>::iterator iter = pluginEngine->m_loadedPlugins.begin();
			iter != pluginEngine->m_loadedPlugins.end(); ++iter )
	{	
		UVDPlugin *plugin = (*iter).second;
		std::string pluginName = (*iter).first;
		
		uv_assert_ret(plugin);
		printf_help("\n");
		printf_help("Plugin %s:\n", pluginName.c_str());
		
		for( std::map<UVDArgConfig *, std::string>::iterator iter = pluginEngine->m_pluginArgMap.begin();
				iter != pluginEngine->m_pluginArgMap.end(); ++iter )
		{
			UVDArgConfig *argConfig = (*iter).first;
			std::string currentPluginName = (*iter).second;
			
			uv_assert_ret(argConfig);
			
			if( pluginName == currentPluginName )
			{
				uv_assert_err_ret(argConfig->print(""));
			}
		}
	}
	
	printf_help("\n");
	
	return UV_ERR_OK;
}

void UVDConfig::printHelp()
{
	printVersion();
	printUsage();
}

/*
UVDArgEngine
*/

UVDArgEngine::UVDArgEngine() {
}

UVDArgEngine::~UVDArgEngine() {
}

//Where did we last parse given property from?
uv_err_t UVDArgEngine::getLastSource( const std::string &argProperty, unsigned int *source ) {
	uv_assert_ret(source);
	
	if( m_lastSource.find(argProperty) != m_lastSource.end() ) {
		*source = m_lastSource[argProperty];
	} else {
		//Consider checking to see if the property is registered at all
		*source = UVD_ARG_FROM_DEFAULT; 
	}
	return UV_ERR_OK;
}

uv_err_t UVDArgEngine::setLastSource( const std::string &argProperty, unsigned int source ) {
	m_lastSource[argProperty] = source;
	return UV_ERR_OK;
}

