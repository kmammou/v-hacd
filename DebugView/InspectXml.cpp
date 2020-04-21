#include "InspectXml.h"
#include "FastXml.h"
#include "StringHelper.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <stdlib.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <assert.h>

#pragma warning(disable:4100)

namespace INSPECT_XML
{

typedef std::unordered_map< std::string, uint32_t > StringToToken;
typedef std::vector< std::string > StringVector;

class InspectXmlImpl :public InspectXml, public FAST_XML::FastXml::Callback
{
public:

	// return true to continue processing the XML document, false to skip.
	virtual bool processElement(
		const char *elementName,   // name of the element
		uint32_t argc,         // number of attributes pairs
		const char **argv,         // list of attributes.
		const char  *elementData,  // element data, null if none
		uint32_t lineno) final  // line number in the source XML file
	{
		if (argc & 1) // if it's odd
		{
			argc--;
		}
		{
			std::string e(elementName);
			StringToToken::iterator found = mElements.find(e);
			if (found == mElements.end())
			{
				mElementCount++;
				mElements[e] = mElementCount;
			}
		}
		uint32_t acount = argc / 2;
		for (uint32_t i = 0; i < acount; i++)
		{
			std::string attr(argv[i * 2]);
			StringToToken::iterator found = mAttributes.find(attr);
			if (found == mAttributes.end())
			{
				mAttributeCount++;
				mAttributes[attr] = mAttributeCount;
			}

		}

		return true;
	}

	// Parses this XML and accumulates all of the unique element and attribute names
	virtual void inspectXml(const char *xmlName) final
	{
		FAST_XML::FastXml *f = FAST_XML::FastXml::create();
		printf("Inspecting XML File: %s", xmlName);
		f->processXml(xmlName, this);
		f->release();
		printf("\n");
	}

	void fixup(char *str)
	{
		while (*str)
		{
			char c = *str;
			if (c == ':') c = '_';
			*str = c;
			str++;
		}
	}
	// Writes out a CPP file which contains an enumeration of all unique
	// element and attribute names as well as lookup tables
	// to rapidly convert ASCII names into enumerated values and the reverse
	virtual void saveSourceCode(const char *sourceCodeName) final
	{
		printf("Saving SourceCode: %s\n", sourceCodeName);
		FILE *fph = fopen(sourceCodeName, "wb");
		if (fph == nullptr) return;
		StringVector elements;
		for (auto &i : mElements)
		{
			char scratch[512];
			strncpy(scratch, i.first.c_str(), 512);
			_strupr(scratch);
			fixup(scratch);

			for (auto &j : elements)
			{
				if (strcmp(j.c_str(), scratch) == 0)
				{
					assert(0);
				}
			}

			elements.push_back(std::string(scratch));
		}
		std::sort(elements.begin(), elements.end());
		//
		fprintf(fph, "// %d unique element types\r\n", int(elements.size()));
		fprintf(fph, "enum ElementType{\r\n");
		for (auto &i : elements)
		{
			fprintf(fph, "    ET_%s,\r\n", i.c_str());
		}
		fprintf(fph, "    ET_LAST\r\n");
		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");

		StringVector attributes;
		for (auto &i : mAttributes)
		{
			char scratch[512];
			strncpy(scratch, i.first.c_str(), 512);
			_strupr(scratch);
			fixup(scratch);

			for (auto &j : attributes)
			{
				if (strcmp(j.c_str(), scratch) == 0)
				{
					assert(0);
				}
			}


			attributes.push_back(std::string(scratch));
		}
		std::sort(attributes.begin(), attributes.end());
		fprintf(fph, "// %d unique attribute types\r\n", int(attributes.size()));
		fprintf(fph, "enum AttributeType{\r\n");
		for (auto &i : attributes)
		{
			fprintf(fph, "    AT_%s,\r\n", i.c_str());
		}
		fprintf(fph, "    AT_LAST\r\n");
		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");

		fprintf(fph, "struct ElementStruct\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	ElementType 	mType;\r\n");
		fprintf(fph, "	const char		*mName;\r\n");
		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");

		fprintf(fph, "static ElementStruct gElements[ET_LAST] =\r\n");
		fprintf(fph, "{\r\n");
		//
		for (auto &i : elements)
		{
			for (auto &j : mElements)
			{
				char scratch[512];
				strncpy(scratch, j.first.c_str(), 512);
				_strupr(scratch);
				fixup(scratch);
				if (strcmp(scratch, i.c_str()) == 0)
				{
					fprintf(fph, "    ET_%s, \"%s\",\r\n", i.c_str(), j.first.c_str());
					break;
				}
			}
		}
		//

		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "struct AttributeStruct\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	AttributeType mType;\r\n");
		fprintf(fph, "	const char	  *mName;\r\n");
		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "static AttributeStruct gAttributes[AT_LAST] =\r\n");
		fprintf(fph, "{\r\n");

		//
		for (auto &i : attributes)
		{
			for (auto &j : mAttributes)
			{
				char scratch[512];
				strncpy(scratch, j.first.c_str(), 512);
				_strupr(scratch);
				fixup(scratch);
				if (strcmp(scratch, i.c_str()) == 0)
				{
					fprintf(fph, "    AT_%s, \"%s\",\r\n", i.c_str(), j.first.c_str());
					break;
				}
			}
		}
		//

		fprintf(fph, "};\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "typedef std::unordered_map< std::string, ElementType > ElementTypeMap;\r\n");
		fprintf(fph, "typedef std::unordered_map< std::string, AttributeType > AttributeTypeMap;\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "ElementTypeMap gElementsMap;\r\n");
		fprintf(fph, "AttributeTypeMap gAttributesMap;\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "static void initMaps(void)\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	for(auto &i:gElements)\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		gElementsMap[ std::string(i.mName) ] = i.mType;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "	for (auto &i:gAttributes)\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		gAttributesMap[ std::string(i.mName) ] = i.mType;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "}\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "static ElementType getElementType(const char *str)\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	ElementType ret = ET_LAST;\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "	ElementTypeMap::iterator found = gElementsMap.find( std::string(str) );\r\n");
		fprintf(fph, "	if ( found != gElementsMap.end() )\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		ret = (*found).second;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "	return ret;\r\n");
		fprintf(fph, "}\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "static AttributeType getAttributeType(const char *str)\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	AttributeType ret = AT_LAST;\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "	AttributeTypeMap::iterator found = gAttributesMap.find( std::string(str) );\r\n");
		fprintf(fph, "	if ( found != gAttributesMap.end() )\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		ret = (*found).second;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "	return ret;\r\n");
		fprintf(fph, "}\r\n");
		fprintf(fph, "static const char *getElementName(ElementType t)\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	const char *ret = \"**UNKONWN-ELEMENT-TYPE**\";\r\n");
		fprintf(fph, "	if ( t < ET_LAST )\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		ret = gElements[t].mName;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "	return ret;\r\n");
		fprintf(fph, "}\r\n");
		fprintf(fph, "\r\n");
		fprintf(fph, "static const char *getAttributeName(AttributeType t)\r\n");
		fprintf(fph, "{\r\n");
		fprintf(fph, "	const char *ret = \"**UNKONWN-ATTRIBUTE-TYPE**\";\r\n");
		fprintf(fph, "	if ( t < AT_LAST )\r\n");
		fprintf(fph, "	{\r\n");
		fprintf(fph, "		ret = gAttributes[t].mName;\r\n");
		fprintf(fph, "	}\r\n");
		fprintf(fph, "	return ret;\r\n");
		fprintf(fph, "}\r\n");


		fclose(fph);
	}

	virtual void release(void) final
	{
		delete this;
	}

	virtual bool processComment(const char *comment)final  // encountered a comment in the XML
	{
		return true;
	}

	// 'element' is the name of the element that is being closed.
	// depth is the recursion depth of this element.
	// Return true to continue processing the XML file.
	// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
	// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
	virtual bool processClose(const char *element, uint32_t depth, bool &isError, uint32_t lineno) final	  // process the 'close' indicator for a previously encountered element
	{
		return true;
	}



	uint32_t		mElementCount{ 0 };
	uint32_t		mAttributeCount{ 0 };
	StringToToken	mElements;
	StringToToken	mAttributes;
};

InspectXml *InspectXml::create(void)
{
	InspectXmlImpl *in = new InspectXmlImpl;
	return static_cast<InspectXml *>(in);
}


}
