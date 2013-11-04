#include "Obj.h"
#include <fstream>

class SceneLoader {
public:
	std::vector<Obj*> loaded_objs;
	std::string data_directory;
	SceneLoader( std::string );
	SceneLoader( void );
	GLint load_file(std::string filename );
};