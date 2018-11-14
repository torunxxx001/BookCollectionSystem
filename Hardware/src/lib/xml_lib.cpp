#include "xml_lib.hpp"

#include <memory>
#include <functional>


std::string c_xml::xmlch_to_string(const XMLCh* from)
{
	size_t in_len = xercesc::XMLString::stringLen(from);
	XMLCh* trans_from = new XMLCh[in_len + 1];

	xercesc::XMLString::copyString(trans_from, from);
	xercesc::XMLString::trim(trans_from);

	XMLSize_t bufferSize = 1024;
	xercesc::XMLTransService::Codes failReason;
	xercesc::XMLTranscoder* transcoder = xercesc::XMLPlatformUtils::fgTransService->makeNewTranscoderFor(
		"UTF-8", failReason, bufferSize, xercesc::XMLPlatformUtils::fgMemoryManager
	);

	in_len = xercesc::XMLString::stringLen(trans_from);
	XMLByte* utf8_buff = new XMLByte[(in_len * 4) + 1];

	unsigned int eaten;
	unsigned int utf8Len = transcoder->transcodeTo(trans_from, in_len,
	utf8_buff, in_len * 4, eaten, xercesc::XMLTranscoder::UnRep_Throw);

	utf8_buff[utf8Len] = '\0';
	std::string result = (char*)utf8_buff;

	delete[] utf8_buff;
	delete transcoder;


	delete[] trans_from;

	return result;
}


c_xml::c_xml(std::string xml_path)
{
	xercesc::XMLPlatformUtils::Initialize();

	std::unique_ptr<xercesc::XercesDOMParser> parser(new xercesc::XercesDOMParser());

	//XMLをロードする
	parser->parse(xml_path.c_str());

	xercesc::DOMDocument* dom = parser->getDocument();
	if( !dom) throw xml_path + ":XMLのロードに失敗しました";

	xercesc::DOMElement* root = dom->getDocumentElement();

	//要素取得用再帰関数
	std::function<std::vector<TAG_ELEMENT> (xercesc::DOMNode *par_node)> get_child;
	get_child = [&](xercesc::DOMNode *par_node){
		std::vector<TAG_ELEMENT> childs;

		xercesc::DOMNodeList* child_nodes = par_node->getChildNodes();

		for(int n_idx = 0; n_idx < child_nodes->getLength(); n_idx++){
			xercesc::DOMNode* node_tmp = child_nodes->item(n_idx);

			if(node_tmp->getNodeType() == xercesc::DOMNode::ELEMENT_NODE){
				TAG_ELEMENT tag_elem_tmp;

				//子要素取得
				tag_elem_tmp.child_elements = get_child(node_tmp);


				//node_tmpの属性一覧取得
				xercesc::DOMNamedNodeMap* attr_list = node_tmp->getAttributes();
				for(int attr_idx = 0; attr_idx < attr_list->getLength(); attr_idx++){
					xercesc::DOMNode* attr_node = attr_list->item(attr_idx);

					std::string attr_name = c_xml::xmlch_to_string(attr_node->getNodeName());
					std::string attr_value = c_xml::xmlch_to_string(attr_node->getNodeValue());

					tag_elem_tmp.attributes[attr_name] = attr_value;
				}

				//タグ名取得
				tag_elem_tmp.tag_name = c_xml::xmlch_to_string(node_tmp->getNodeName());

				//値の取得
				xercesc::DOMNode* elem_text = node_tmp->getFirstChild();
				tag_elem_tmp.value = "";
				if(elem_text){
					tag_elem_tmp.value = c_xml::xmlch_to_string(elem_text->getNodeValue());
				}

				childs.push_back(tag_elem_tmp);
			}
		}

		return childs;
	};

	//ルートを含めた要素取得
	this->child_elements = get_child(root->getParentNode());
}

c_xml::TAG_LIST_PTR c_xml::get_root()
{
	return &this->child_elements;
}
