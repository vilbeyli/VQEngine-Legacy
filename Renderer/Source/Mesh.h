#pragma once

struct Vertex;

// refactor mesh
class Mesh
{
	//Mesh() : vertices(NULL), numVertices(0), indices(NULL), numIndices(0) {}
	//~Mesh() { if (vertices) delete[] vertices; if (indices) delete[] indices; }
	//Vertex* vertices;
	//unsigned numVertices;
	//unsigned* indices;
	//unsigned numIndices;
};

enum MESH_TYPE
{
	TRIANGLE = 0,
	QUAD,
	CUBE,
	SPHERE,
	COUNT
};