/*==================================================================================================

 NamingKey.cpp

 purpose: Provide support for an art content naming key

 author:  biddle

 todo:	  There is a possible nasty buffer overflow problem. Need to replace the strcats
          with something safer

		  Should create a list of trees mapped to there first letter. Much more flexible.


 (c)opyright Gas Powered Games, 1999

---------------------------------------------------------------------------------------------------*/
#include "precomp_gpcore.h"
#include "namingkey.h"
#include "filesys.h"
#include "filesysutils.h"
#include "stdhelp.h"
#include "stringtool.h"

#define LINE_LENGTH 256
#define TRIMMED_SIZE 80

//****************************************************************************
NamingKey::NamingKey()
	: m_BitMapTree(0)
	, m_MeshTree(0)
	, m_AnimTree(0)
	, m_TerrainTree(0)
	, m_SoundTree(0)
	, m_SkritTree(0)
{
}

//****************************************************************************
NamingKey::NamingKey(const char* dirname)
	: m_BitMapTree(0)
	, m_MeshTree(0)
	, m_AnimTree(0)
	, m_TerrainTree(0)
	, m_SoundTree(0)
	, m_SkritTree(0)
{
	LoadNamingKey(dirname);
}

//****************************************************************************
bool NamingKey::LoadNamingKey(const char* fname)
{

	// Locate all the files in "dirname" with an NNK extension

	gpstring pathSpec = FileSys::GetPathOnly( fname ) + "*.NNK";

	gpgenericf(("Initializing NamingKey, source directory %s\n",pathSpec.c_str()));

	std::list <gpstring> stockFiles;
	std::list <gpstring> extraFiles;

	FileSys::AutoFindHandle finder( pathSpec );
	gpstring fileName;
	while ( finder.GetNext( fileName, true ) )
	{
		if (FileSys::GetFileNameOnly( fileName ).same_no_case("NAMINGKEY",sizeof("NAMINGKEY")-1) )
		{
			stockFiles.push_back( fileName );
		}
		else
		{
			// This is a namingkey provided by a mod maker
			extraFiles.push_back( fileName );
		}
	}

	stockFiles.sort(istring_less());
	extraFiles.sort(istring_less());

	if (stockFiles.empty())
	{
		gpgeneric(" ...FAILED TO LOCATE ANY NAMING KEY FILES AT ALL!\n");
		return false;
	}

	// Only parse the LAST NamingKeyXXX.NNK
	gpstring fullName = stockFiles.back();

	gpgenericf(("  loading STANDARD file: %s\n",fullName.c_str()));

	FileSys::AutoFileHandle file;
	FileSys::AutoMemHandle mem;

	if ( !file.Open( fullName ) || !mem.Map( file ) )
	{
		gpgeneric(" ...FAILED TO OPEN STANDARD NAMING KEY!\n");
	}
	else
	{
		m_FileNames.clear();

		delete m_BitMapTree;	m_BitMapTree = 0;  
		delete m_MeshTree;		m_MeshTree = 0;
		delete m_AnimTree;		m_AnimTree = 0;
		delete m_TerrainTree;	m_TerrainTree = 0;
		delete m_SoundTree;		m_SoundTree = 0;
		delete m_SkritTree;		m_SkritTree = 0;
		
		int parsecount = 0;

		if (!ParseNamingKey( rcast <const char*> ( mem.GetData() ), mem.GetSize() , parsecount))
		{
			gpgenericf(("\t\t...encounted errors, parsed %d standard entries\n",parsecount));
		}
		else
		{
			gpgenericf(("\t\t...succeeded, parsed %d standard entries\n",parsecount));

			m_FileNames.push_back(fullName);

			std::list<gpstring>::iterator fbeg = extraFiles.begin();
			std::list<gpstring>::iterator fend = extraFiles.end();
			std::list<gpstring>::iterator foundNameIt;

			for (foundNameIt = fbeg; foundNameIt != fend; ++foundNameIt)
			{
				
				fullName = (*foundNameIt);

				gpgenericf(("  loading EXTRA file: %s\n",fullName.c_str()));

				mem.Close();
				if ( !file.Open( fullName ) || !mem.Map( file ) )
				{
					gpgeneric(" ...FAILED TO OPEN EXTRA NAMING KEY!\n");
				}
				else
				{
					int parsecount = 0;
					if (ParseNamingKey( rcast <const char*> ( mem.GetData() ), mem.GetSize() , parsecount))
					{
						gpgenericf(("\t\t...succeeded, parsed %d extra entries\n",parsecount));
					}
					else
					{
						gpgenericf(("\t\t...encounted errors, parsed %d extra entries\n",parsecount));
					}
					m_FileNames.push_back(fullName);
				}
				
			}
		}
		mem.Close();
	}

	return m_FileNames.size() > 0;
}

//****************************************************************************
NamingKey::~NamingKey(void)
{
	delete m_BitMapTree;
	delete m_MeshTree;
	delete m_AnimTree;
	delete m_TerrainTree;
	delete m_SoundTree;
	delete m_SkritTree;
}


//****************************************************************************
bool NamingKey::BuildASPLocation(const char *candidate,gpstring &dest) const
 {

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	dest = "Art\\";

	if (upper_candidate[0] != 'M') return false;

	if (BuildTreePath(m_MeshTree,upper_candidate,dest)) {

		dest += candidate;
		dest += ".ASP";
		return true;

	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildPRSLocation(const char *candidate,gpstring &dest) const
 {

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	gpstring abbr,group,animclass,animtype,extra;

	dest.clear();

	// Still support the 'old' naming convention as the default

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'A') return false;

	dest = "Art\\";

	if (BuildTreePath(m_AnimTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".PRS";
		return true;
	}

	return false;

}

//****************************************************************************
bool NamingKey::BuildIMGLocation(const char *candidate,gpstring &dest) const {

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'B') return false;

	dest = "Art\\";

	if (BuildTreePath(m_BitMapTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".%img%";
		return true;
	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildWAVLocation(const char *candidate,gpstring &dest) const
{

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'S') return false;

	dest.clear();

	if (BuildTreePath(m_SoundTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".WAV";
		return true;
	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildMP3Location(const char *candidate,gpstring &dest) const
{

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'S') return false;

	dest.clear();

	if (BuildTreePath(m_SoundTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".MP3";
		return true;
	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildSkritLocation(const char *candidate,gpstring &dest) const
{

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'K') return false;

	dest.clear();

	if (BuildTreePath(m_SkritTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".skrit";
		return true;
	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildSNOLocation(const char *candidate,gpstring &dest) const {

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	if (upper_candidate[0] != 'T') return false;

	dest = "Art\\";

	if (BuildTreePath(m_TerrainTree,upper_candidate,dest))
	{
		dest += candidate;
		dest += ".SNO";
		return true;
	}

	return false;
}

//****************************************************************************
bool NamingKey::BuildContentPath(const char *candidate,gpstring &dest) const
{

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);


	bool found = false;

	if (upper_candidate[0] == 'B')
	{
		dest = "Art\\";
		found = BuildTreePath(m_BitMapTree,upper_candidate,dest);
	}
	else if (upper_candidate[0] == 'M')
	{
		dest = "Art\\";
		found = BuildTreePath(m_MeshTree,upper_candidate,dest);
	}
	else if (upper_candidate[0] == 'A')
	{
		dest = "Art\\";
		found = BuildTreePath(m_AnimTree,upper_candidate,dest);
	}
	else if (upper_candidate[0] == 'T')
	{
		dest = "Art\\";
		found = BuildTreePath(m_TerrainTree,upper_candidate,dest);
	}
	else if (upper_candidate[0] == 'S')
	{
		dest.clear();
		found = BuildTreePath(m_SoundTree,upper_candidate,dest);
	}
	else if (upper_candidate[0] == 'K')
	{
		dest.clear();
		found = BuildTreePath(m_SkritTree,upper_candidate,dest);
	}

	if (found)
	{
		return true;
	}

	dest.clear();
	return false;
}

//****************************************************************************
bool NamingKey::BuildContentLocation(const char *candidate,gpstring &dest) const
{

	bool found = BuildContentPath(candidate,dest);

	if (found)
	{
		dest += candidate;
		return true;
	}

	dest.clear();
	return false;
}

//****************************************************************************
bool NamingKey::BuildContentDescription(const char *candidate,std::list<gpstring> &desc) const
{

	desc.clear();

	if (!candidate) return false;
	if (!candidate[0]) return false;
	if (!candidate[1]) return false;
	if (candidate[1] != '_') return false;

	char upper_candidate[TRIMMED_SIZE];

	stringtool::CopyString(upper_candidate,candidate,TRIMMED_SIZE);
	_strupr(upper_candidate);

	bool found = false;

	if (upper_candidate[0] == 'B')
	{
		found = BuildTreeDesc(m_BitMapTree,upper_candidate,desc);
	}
	else if (upper_candidate[0] == 'M')
	{
		found = BuildTreeDesc(m_MeshTree,upper_candidate,desc);
	}
	else if (upper_candidate[0] == 'A')
	{
		found = BuildTreeDesc(m_AnimTree,upper_candidate,desc);
	}
	else if (upper_candidate[0] == 'T')
	{
		found = BuildTreeDesc(m_TerrainTree,upper_candidate,desc);
	}
	else if (upper_candidate[0] == 'S')
	{
		found = BuildTreeDesc(m_SoundTree,upper_candidate,desc);
	}
	else if (upper_candidate[0] == 'K')
	{
		found = BuildTreeDesc(m_SkritTree,upper_candidate,desc);
	}

	if (found)
	{
		return true;
	}

	desc.clear();
	return false;
}



//****************************************************************************
NamingKey::ParseError NamingKey::InsertNode(NameNode* &tree,const char* searchabbr, const gpstring& name, const gpstring& desc)
{

	if (!tree)
	{
		tree = new NameNode(searchabbr,name,desc,NULL);
		if (tree)
		{
			return NNK_SUCCEEDED;
		}
		else
		{
			return NNK_ERROR_COULD_NOT_CREATE;
		}
	}

	if (tree->m_children.size() == 0)
	{

		if (strcmp(searchabbr,tree->m_abbr.c_str()) == 0)
		{
			return NNK_ERROR_DUPLICATE;	// Already exists, subsequent loads are simply ignored
		}

		char* nextnodeabbr = strchr(searchabbr,'_');

		if (nextnodeabbr)
		{
			unsigned int len = nextnodeabbr - searchabbr;
			nextnodeabbr++;
			while ( *nextnodeabbr == '_' )
			{
				nextnodeabbr++;
			}

			if ( (tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0) )
			{
				NameNode* kid = new NameNode(nextnodeabbr,name,desc,tree);
				if (kid)
				{
					return NNK_SUCCEEDED;
				}
				else
				{
					return NNK_ERROR_COULD_NOT_CREATE;
				}
			}
		}

		return NNK_ERROR_NO_MATCH;	// No match

	}
	
	char* nextnodeabbr = strchr(searchabbr,'_');

	if (nextnodeabbr)
	{

		unsigned int len = nextnodeabbr - searchabbr;
		nextnodeabbr++;
		while ( *nextnodeabbr == '_' )
		{
			nextnodeabbr++;
		}

		if ((tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0) )
		{
			unsigned int len = strcspn(nextnodeabbr,"_");

			for (NameNode::NameNodeIter i = tree->m_children.begin(); i != tree->m_children.end(); ++i) 
			{
				if ( ((*i)->m_abbr.size() == len) && (strncmp(nextnodeabbr,(*i)->m_abbr.c_str(),len) == 0) )
				{
					return InsertNode((*i), nextnodeabbr,name,desc);
				}
			}

			NameNode* kid = new NameNode(nextnodeabbr,name,desc,tree);
			if (kid)
			{
				return NNK_SUCCEEDED;
			}
			else
			{
				return NNK_ERROR_COULD_NOT_CREATE;
			}
		}
	}

	if (tree->m_children.size())
	{
		return NNK_ERROR_DUPLICATE;	// This is a duplicate of an internal node
	}

	return NNK_ERROR_ILLEGAL;

}

//****************************************************************************
void NamingKey::DumpTrees(void) const
{
#if !GP_RETAIL
	if (DumpTree(m_BitMapTree,0))	gpgeneric("\n");
	if (DumpTree(m_MeshTree,0))		gpgeneric("\n");
	if (DumpTree(m_AnimTree,0))		gpgeneric("\n");
	if (DumpTree(m_TerrainTree,0))	gpgeneric("\n");
	if (DumpTree(m_SoundTree,0))	gpgeneric("\n");
	if (DumpTree(m_SkritTree,0))	gpgeneric("\n");
#endif //!GP_RETAIL
}

//****************************************************************************
bool NamingKey::DumpTree(NameNode* tree, int spacing) const
{

#if GP_RETAIL

	UNREFERENCED_PARAMETER(tree);
	UNREFERENCED_PARAMETER(spacing);

#else

	if (!tree) return false;

	{for (int i = 0; i < spacing; i++) {
		gpgeneric(" ");
	}}

	gpgenericf(( "%s:%s [%s]\n",tree->m_abbr.c_str(),tree->m_name.c_str(),tree->m_desc.c_str() ));

	for (NameNode::NameNodeIter i = tree->m_children.begin(); i != tree->m_children.end(); ++i) {
		DumpTree((*i),spacing+1);
	}

#endif	//GP_SIEGEMAX

	return true;
}


//****************************************************************************
bool NamingKey::BuildTreePath(NameNode* tree,const char *searchabbr, gpstring &path) const {

	if (!tree)
	{
		return false;
	}

	if ( !tree->m_name.empty() )
	{
		path += tree->m_name;
		path += "\\";
	}

	if (tree->m_children.size() == 0) {

		if (strcmp(searchabbr,tree->m_abbr.c_str()) == 0) {
			return true;
		}

		char* nextnodeabbr = strchr(searchabbr,'_');

		if (nextnodeabbr) {

			unsigned int len = nextnodeabbr - searchabbr;
			if ((tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0))
			{
				return true;
			}
		}

		return false;	// No match

	}

	char* nextnodeabbr = strchr(searchabbr,'_');

	if (nextnodeabbr) {

		unsigned int len = nextnodeabbr - searchabbr;

		nextnodeabbr++;
		while ( *nextnodeabbr == '_' )
		{
			nextnodeabbr++;
		}

		if ((tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0))
		{
			len = strcspn(nextnodeabbr,"_");

			for (NameNode::NameNodeIter i = tree->m_children.begin(); i != tree->m_children.end(); ++i) 
			{
				if (((*i)->m_abbr.size() == len) && (strncmp(nextnodeabbr,(*i)->m_abbr.c_str(),len) == 0)) 
				{
					return (BuildTreePath(*i, nextnodeabbr, path));
				}
			}

			return true;

		}
	}

	return false;


}

//****************************************************************************
bool NamingKey::BuildTreeDesc(NameNode* tree,const char *searchabbr, std::list<gpstring> &desc) const {

	if (!tree)
	{
		return false;
	}

	if ( !tree->m_name.empty() )
	{
		desc.push_back(tree->m_desc);
	}

	if (tree->m_children.size() == 0) {

		if (strcmp(searchabbr,tree->m_abbr.c_str()) == 0) {
			return true;
		}

		char* nextnodeabbr = strchr(searchabbr,'_');

		if (nextnodeabbr) {

			unsigned int len = nextnodeabbr - searchabbr;
			if ((tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0))
			{
				return true;
			}
		}

		return false;	// No match

	}

	char* nextnodeabbr = strchr(searchabbr,'_');

	if (nextnodeabbr) {

		unsigned int len = nextnodeabbr - searchabbr;

		nextnodeabbr++;
		while ( *nextnodeabbr == '_' )
		{
			nextnodeabbr++;
		}

		if ((tree->m_abbr.size() == len) && (strncmp(searchabbr,tree->m_abbr.c_str(),len) == 0))
		{
			len = strcspn(nextnodeabbr,"_");

			for (NameNode::NameNodeIter i = tree->m_children.begin(); i != tree->m_children.end(); ++i) 
			{
				if (((*i)->m_abbr.size() == len) && (strncmp(nextnodeabbr,(*i)->m_abbr.c_str(),len) == 0)) 
				{
					return (BuildTreeDesc(*i, nextnodeabbr, desc));
				}
			}

			return true;

		}
	}

	return false;

}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

void trimcopy(char *newstr,const char *str) {

	bool leading_quote = false;

	if (str == NULL) {
		newstr[0] = 0;
		return;
	}

	while (isspace(*str)) {
		str++;
	}

	if (*str == '\"') {
		leading_quote = true;
		str++;
	}

	int i = strlen(str);

	while (i > 0 && (isspace(str[i-1]) || *str == '\"')) {
		i--;
	}

	if (leading_quote && (str[i-1] == '\"')) {
		i--;
	}

	stringtool::CopyString(newstr,str,TRIMMED_SIZE,i);
}

//****************************************************************************
const char* NamingKey::LookupErrorCode(ParseError err)
{
	switch (err)
	{
		case NNK_SUCCEEDED				: return "NNK_SUCCEEDED";
		case NNK_ERROR_NO_MATCH			: return "NNK_ERROR_NO_MATCH";
		case NNK_ERROR_ILLEGAL			: return "NNK_ERROR_ILLEGAL";
		case NNK_ERROR_DUPLICATE		: return "NNK_ERROR_DUPLICATE";
		case NNK_ERROR_COULD_NOT_CREATE	: return "NNK_ERROR_COULD_NOT_CREATE";
		case NNK_ERROR_UNKNOWN_TREE		: return "NNK_ERROR_UNKNOWN_TREE";
	}
	return "NNK_UKNOWN_ERROR_CODE";
}

//****************************************************************************
bool NamingKey::ParseNamingKey(const char *buf,int buflen, int& nodecount)
{

	char linebuf[LINE_LENGTH];
	int linecount = 1;

	nodecount = 0;
	bool found_error = false;

	int p=0;

	while (p < buflen)
	{

		// Read a line
		int j = 0;
		linebuf[0]=0;

		bool comment_found = false;

		char c = buf[p++];
		if (c == '\n') linecount++;

		while ((c == '\r' || c == '\n') && p < buflen)
		{
			c = buf[p++];
			if (c == '\n') linecount++;
		}

		while (c != '\r' && c != '\n' && p < buflen )
		{
			if (!comment_found)
			{
				if (c == '#')
				{
					comment_found = true;
				}
				else if ( (c == '/') && (buf[p] == '/') ) 
				{
					p++;
					comment_found = true;
				}
				else
				{
					if (j < LINE_LENGTH-1) 
					{
						linebuf[j++] = c;
					}
				}
			}
			c = buf[p++];
			if (c == '\n')linecount++;
		}

		// Skip lines that are nothing but a comment
		if (j == 0) continue;

		linebuf[j] = 0;

		gpstring origline(linebuf);

		// Parse the line

		// No doubt there is cleaner way to do this using STL, but I am porting from MAXSCRIPT

		char abbr[TRIMMED_SIZE];
		char name[TRIMMED_SIZE];
		char desc[TRIMMED_SIZE];
		char parent[TRIMMED_SIZE];

		char token[256];
		char* rawtoken;

		rawtoken = strtok(linebuf,"=");
		trimcopy(token,rawtoken);

		if (token[0] != NULL) {

			abbr[0] = 0;
			name[0] = 0;
			desc[0] = 0;
			parent[0] = 0;

			_strupr(token);

			if (strcmp("TREE",token) == 0) {

				rawtoken = strtok(NULL,",");
				trimcopy(abbr,rawtoken);
				rawtoken = strtok(NULL,",");
				trimcopy(name,rawtoken);
				rawtoken = strtok(NULL,",");
				trimcopy(desc,rawtoken);

				_strupr(abbr);
				char *searchabbr = abbr;
				if (searchabbr[1] == 0 || searchabbr[1] == '_')  
				{
					ParseError insertok = NNK_ERROR_UNKNOWN_TREE;

					switch (*searchabbr)
					{
					case 'B' :
						insertok = InsertNode(m_BitMapTree,searchabbr,name,desc);
						break;
					case 'S' :
						insertok = InsertNode(m_SoundTree,searchabbr,name,desc);
						break;
					case 'K' :
						insertok = InsertNode(m_SkritTree,searchabbr,name,desc);
						break;
					case 'M' :
						insertok = InsertNode(m_MeshTree,searchabbr,name,desc);
						break;
					case 'T' :
						insertok = InsertNode(m_TerrainTree,searchabbr,name,desc);
						break;
					case 'A' :
						insertok = InsertNode(m_AnimTree,searchabbr,name,desc);
						if (insertok == NNK_SUCCEEDED)
						{
							// Handle the extra FS0,FS1... suffixes that may appear in an animation tree node
							// We will re-use the name and desc buffers (they are no longer valid at this point)
							rawtoken = strtok(NULL,",");
							trimcopy(desc,rawtoken);
							while (*desc != 0) {
								_strupr(desc);
								strcpy(name,abbr);
								strcat(name,"_");
								strcat(name,desc);
								ParseError extraok = InsertNode(m_AnimTree,name,desc,desc);
								if ( extraok == NNK_SUCCEEDED )
								{
									nodecount++;
								}
								else
								{
									gpwarningf( ("\t\t[%s] at line %d: %s\n",LookupErrorCode(extraok),linecount,origline.c_str()) );
									found_error = true;
								}
								rawtoken = strtok(NULL,",");
								trimcopy(desc,rawtoken);
							}
						}
						break;
					}
					if (insertok == NNK_SUCCEEDED) 
					{
						nodecount++;
					}
					else
					{
						gpwarningf( ("\t\t[%s] at line %d: %s\n",LookupErrorCode(insertok),linecount,origline.c_str()) );
						found_error = true;
					}
				}
			}
		}
	}

	return !found_error;

}
