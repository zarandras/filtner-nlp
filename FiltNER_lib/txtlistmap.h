#ifndef TXTLISTMAP_H
#define TXTLISTMAP_H

#include <fstream>
#include <set>
#include <map>
#include <string>

typedef std::set<std::string>		StringSet;

class TxtListMap {
private:
	typedef std::map<std::string, StringSet *> TLMKernel;
	TLMKernel	theMap;

	inline	void	readNextTxt (std::istream &listFile, std::string &dest)		{ listFile >> dest; }
	inline	void	insertTxt (const std::string &txt, StringSet	&dest)			{ dest.insert (txt); }
public:
					// default constructor generated
	const	StringSet	*getTxtList (const std::string &typeName) const;
						~TxtListMap ();
			void		 addTxtList (const std::string &typeName, const std::string &fileName);
								// reads list from file
};

#endif /* TXTLISTMAP_H */
