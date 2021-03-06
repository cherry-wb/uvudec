/*
UVNet Universal Decompiler (uvudec)
Copyright 2010 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#include "testing/obj2pat.h"
#include "uvd/core/uvd.h"
#include "uvdflirt/flirt.h"
#include "uvdflirt/plugin.h"
#include <string.h>

CPPUNIT_TEST_SUITE_REGISTRATION(UVDObj2patUnitTest);

void UVDObj2patUnitTest::cppTest()
{
	verifyObj2Pat("cpp");
}

void UVDObj2patUnitTest::inlineTest()
{
	verifyObj2Pat("inline");
}

void UVDObj2patUnitTest::noNameTest()
{
	verifyObj2Pat("no_name");
}

void UVDObj2patUnitTest::noNamesTest()
{
	verifyObj2Pat("no_names");
}

void UVDObj2patUnitTest::recursiveTest()
{
	verifyObj2Pat("recursive");
}

void UVDObj2patUnitTest::shortNamesTest()
{
	verifyObj2Pat("short_names");
}

void UVDObj2patUnitTest::shortNamesRefTest()
{
	verifyObj2Pat("short_names_ref");
}

void UVDObj2patUnitTest::shortTest()
{
	verifyObj2Pat("short");
}

void UVDObj2patUnitTest::testingMainTest()
{
	verifyObj2Pat("uvtest_main");
}

void UVDObj2patUnitTest::libmTest()
{
	verifyObj2Pat("libm.a", "libm.pat");
}

/*
Utility
*/

char safePrintChar(char c)
{
	if( isprint(c) )
	{
		return c;
	}
	else
	{
		return '.';
	}
}

void UVDObj2patUnitTest::verifyObj2Pat(const std::string &filePrefix)
{
	verifyObj2Pat(filePrefix + ".o", filePrefix + ".pat");
}

void UVDObj2patUnitTest::verifyObj2Pat(const std::string &objectFileNameIn, const std::string &expectedPatFileNameIn)
{
	std::string tempFile;

	std::string unitTestDir;
	//std::string outputPatFileName = getTempFileName();
	std::string outputPatFileContents;
	std::string expectedPatFileContents;
	std::string objectFileName;
	std::string expectedPatFileName;
	
	unitTestDir = getUnitTestDir();
	objectFileName = unitTestDir + "/flirt/ELF/" + objectFileNameIn;
	expectedPatFileName = unitTestDir + "/flirt/ELF/" + expectedPatFileNameIn;

	m_args.clear();
	m_uvdInpuFileName = objectFileName;
	generalInit();
	UVCPPUNIT_ASSERT(g_uvdFLIRTPlugin->m_flirt->toPat(outputPatFileContents));
	deinit();
	
	//UVCPPUNIT_ASSERT(readFile(outputPatFileName, outputPatFileContents));
	UVCPPUNIT_ASSERT(readFile(expectedPatFileName, expectedPatFileContents));
	try
	{
		CPPUNIT_ASSERT_EQUAL(expectedPatFileContents, outputPatFileContents);
	}
	catch(...)
	{
		for( std::string::size_type i = 0; i < expectedPatFileContents.size() && i < outputPatFileContents.size(); ++i )
		{
			if( expectedPatFileContents[i] != outputPatFileContents[i] )
			{
				printf("difference at offset %d, %c/0x%02X (expected) vs %c/0x%02X (given)\n",
						i,
						safePrintChar(expectedPatFileContents[i]), expectedPatFileContents[i],
						safePrintChar(outputPatFileContents[i]), outputPatFileContents[i]);
				break;
			}
		}
		throw;
	}
}

