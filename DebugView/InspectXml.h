#ifndef INSPECT_XML_H
#define INSPECT_XML_H

// This code snippet will parse an XML and produce a sorted
// list of all unique element names which were found
// as well as all unique attribute names
// It will produce a snippet of source code which create an enumeration
// of all of these types and helper methods to convert from ASCII to enum value
// and from enum value back to ASCII

namespace INSPECT_XML
{

class InspectXml
{
public:
	static InspectXml *create(void);

	// Parses this XML and accumulates all of the unique element and attribute names
	virtual void inspectXml(const char *xmlName) = 0;

	// Writes out a CPP file which contains an enumeration of all unique
	// element and attribute names as well as lookup tables
	// to rapidly convert ASCII names into enumerated values and the reverse
	virtual void saveSourceCode(const char *sourceCodeName) = 0;

	virtual void release(void) = 0;
};

}

#endif
