#ifndef __XML_LIB_HPP__
#define __XML_LIB_HPP__

#include <vector>
#include <map>
#include <string>

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/util/TransService.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

typedef std::map<std::string, std::string> TAG_ATTR_LIST;

typedef struct _tag_element {
	std::string tag_name;
	std::string value;

	std::vector<_tag_element> child_elements;
	TAG_ATTR_LIST attributes;
} TAG_ELEMENT;

class c_xml {
	std::vector<TAG_ELEMENT> child_elements;
public:
	typedef std::vector<TAG_ELEMENT>* TAG_LIST_PTR;

	static std::string xmlch_to_string(const XMLCh* from);


	c_xml(std::string xml_path);

	TAG_LIST_PTR get_root();
};

#endif
