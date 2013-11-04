#include "SceneLoader.h"
#include <fstream>
#include <stdio.h>

using namespace std;
using namespace Angel;

SceneLoader::SceneLoader( string data_dir ) {
	data_directory = data_dir;
}

SceneLoader::SceneLoader( void ) {
	data_directory = "./Data/";
}

GLint SceneLoader::load_file( string filename )
{
	ifstream input_scene_file;
	string line;
	string filepath;
	GLint status = -1;
	// Clear out old object models
	loaded_objs.clear();
	input_scene_file.open( data_directory + filename);
	if(input_scene_file.is_open())
	{
		// First line is a comment of how many obj files in
		// scene file, but we will just read to EOF found
		getline(input_scene_file, line);
		while(!input_scene_file.eof())
		{
			getline(input_scene_file, line);
			loaded_objs.push_back( new Obj( data_directory + line ) );
		}
		status = 0;
		input_scene_file.close();
	} else {
		status = -1;
	}
	return status;
}