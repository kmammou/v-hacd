#ifndef FAST_XML_H
#define FAST_XML_H

#include <stdint.h>

// This is a very simple XML parser. 
// Designed to be reasonably fast and extremely lightweight.
// Less than half the code size of TinyXml2, but then again doesn't
// have nearly the same level of features either.

namespace FAST_XML
{

class FastXml
{
public:
	/***
	* Callbacks to the user with the contents of the XML file properly digested.
	*/
	class Callback
	{
	public:

		virtual bool processComment(const char *comment) = 0; // encountered a comment in the XML

		// 'element' is the name of the element that is being closed.
		// depth is the recursion depth of this element.
		// Return true to continue processing the XML file.
		// Return false to stop processing the XML file; leaves the read pointer of the stream right after this close tag.
		// The bool 'isError' indicates whether processing was stopped due to an error, or intentionally canceled early.
		virtual bool processClose(const char *element,uint32_t depth,bool &isError,uint32_t lineno) = 0;	  // process the 'close' indicator for a previously encountered element

		// return true to continue processing the XML document, false to skip.
		virtual bool processElement(
			const char *elementName,   // name of the element
			uint32_t argc,         // number of attributes pairs
			const char **argv,         // list of attributes.
			const char  *elementData,  // element data, null if none
			uint32_t lineno) = 0;  // line number in the source XML file

		// process the XML declaration header
		virtual bool processXmlDeclaration(
			uint32_t /*argc*/,
			const char ** /*argv*/,
			const char  * /*elementData*/,
			uint32_t /*lineno*/)
		{
			return true;
		}

		virtual bool processDoctype(
			const char * /*rootElement*/, //Root element tag
			const char * /*type*/,        //SYSTEM or PUBLIC
			const char * /*fpi*/,         //Formal Public Identifier
			const char * /*uri*/)         //Path to schema file
		{
			return true;
		}

	};

	// Creates an instance of the FastXml parser
	static FastXml *create(void);

	// Process an XML from a file on disk; uses standard file IO
	virtual bool processXml(const char *fileName,Callback *iface) = 0;
	// Process an XML that has been pre-loaded into memory
	virtual bool processXml(const void *data,uint32_t dataLen, Callback *iface) = 0;

	virtual const char *getError(uint32_t &lineno) = 0; // report the reason for a parsing error, and the line number where it occurred.

	virtual void release(void) = 0;
};

const char *getAttribute(const char *attr, uint32_t argc, const char **argv);


}; // end of namespace FAST_XML

#endif // FAST_XML_H
