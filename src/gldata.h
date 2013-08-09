#ifndef LDFORGE_GLDATA_H
#define LDFORGE_GLDATA_H

#include "types.h"
#include <QMap>

class QColor;
class LDFile;

class VertexCompiler {
public:
	enum ColorType {
		Normal,
		BFCFront,
		BFCBack,
		PickColor,
	};
	
	enum MergedArrayType {
		MainArray,
		EdgeArray,
		BFCArray,
		PickArray,
		EdgePickArray,
		NumArrays
	};
	
	struct Vertex {
		float x, y, z;
		uint32 color;
		float pad[4];
	};
	
	class Array {
	public:
		typedef int32 SizeType;
		typedef Vertex DataType;
		
		Array();
		Array (const Array& other) = delete;
		~Array();
		
		void clear();
		void merge (Array* other);
		void resizeToFit (SizeType newSize);
		const SizeType& allocatedSize() const;
		SizeType writtenSize() const;
		const DataType* data() const;
		void write (DataType f);
		Array& operator= (const Array& other) = delete;
		
	private:
		DataType* m_data;
		DataType* m_ptr;
		SizeType m_size;
	};
	
	VertexCompiler();
	~VertexCompiler();
	void setFile (LDFile* file);
	void compileFile();
	void compileObject (LDObject* obj);
	void forgetObject (LDObject* obj);
	Array* getMergedBuffer (MergedArrayType type);
	QColor getObjectColor (LDObject* obj, ColorType list) const;
	
private:
	void compilePolygon (LDObject* drawobj, LDObject* trueobj);
	void compileVertex (vertex v, QColor col, VertexCompiler::Array* array);
	
	QMap<LDObject*, Array**> m_objArrays;
	Array* m_mainArrays[NumArrays];
	LDFile* m_file;
	bool m_changed[NumArrays];
};

#endif // LDFORGE_GLDATA_H