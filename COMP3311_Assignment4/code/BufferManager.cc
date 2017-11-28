#include "BufferManager.h"
#include <iostream>

using namespace std;

map <tag, bufferIter> BufferManager::inmemory_table;
list <Block> BufferManager::buffer;

/*TODO 2
input：1.filename 2.read start address(must be int*BLOCKSIZE)
HIT:
    check the data wanted exist in the memory or not, 
    if is, return the data
    if is not, load the data from the db file;
    when the buffer memory goes out,i.e., need to throw some other blocks to make space for the new block
    use the LRU(least recently used) schema.
return：bufferIter. 
*/

bufferIter BufferManager::BufferManagerRead(const string &fileName, long offset)
{
	tableIter it = inmemory_table.find(make_pair(fileName,offset));

	
	if (it!=inmemory_table.end()){ //already in the buffer
        

		Block copy = *(it->second);

		//copy the block, push the copy, remove the original block

	
		buffer.erase(it->second);
		buffer.push_back(copy);

		bufferIter new_block = buffer.end();
		new_block--;
		tag T = make_pair(copy.fileName, copy.offset);
		inmemory_table.find(T)->second = new_block;
		return new_block;
	}
	else{ //not in the buffer
       
		FILE *fp = fopen(fileName.c_str(), "rb");

		if (!fp) {
			cerr << "ERROR: No file named: " <<fileName << " .\n";
			
		}

		fseek(fp, offset, SEEK_SET);

		Block input;
		input.fileName = fileName;
		input.offset = offset;
		input.status = 0;
		
		fread(&input.data, 1, BLOCKSIZE, fp);
		fclose(fp);
		buffer.push_back(input);


		bufferIter new_block = buffer.end();
		new_block--;
		tag T = make_pair(input.fileName, input.offset);
		inmemory_table.insert(make_pair(T, new_block));
        if (buffer.size()>BUFFERSIZE){ //we should use LRU replace
           
			bufferIter victim = buffer.begin();
			
			while (victim->status) {
				victim++;
			}

			if (victim == --buffer.end()) {
				cerr << "No available space to replace!" << endl;
				buffer.pop_back();
				return buffer.end();
			}
			else {
				BufferManagerWrite(*victim);
			}

        }
		return new_block;
	}

	
}

void BufferManager::BufferManagerPin(Block &b)
{
    b.status = 1;
}

void BufferManager::BufferManagerWrite(const Block &b)
{
    FILE *fp = fopen(b.fileName.c_str(), "rb");
    FILE *outFile;
    if (fp==NULL){
        outFile = fopen(b.fileName.c_str(), "wb");
    }
    else{
        fclose(fp);
        outFile = fopen(b.fileName.c_str(), "rb+");
    }
    if(outFile==NULL){
        cerr<<"ERROR: Open file "<<b.fileName<<" failed.\n";
        return;
    }
    fseek(outFile, b.offset, SEEK_SET);  //SEEK_SET -> beginning of the file
    fwrite(&b.data, 1, BLOCKSIZE, outFile);
    fclose(outFile);

    tag T = make_pair(b.fileName, b.offset);
    tableIter tmp = inmemory_table.find(T);
    if (tmp!=inmemory_table.end()){ //in the buffer, which means not new written
        bufferIter victim = tmp->second;
        inmemory_table.erase(inmemory_table.find(T)); //remove from table
        buffer.erase(victim); //remove from buffer
    }

}

int BufferManager::BufferManagerGetStatus(const Block &b)
{
    return b.status;
}

void BufferManager::BufferManagerFlush()
{
    for (bufferIter T = buffer.begin(); T!=buffer.end();){
        BufferManagerWrite(*T++);
    }
    buffer.clear();
    inmemory_table.clear();
}

void BufferManager::deleteFile(const string &fileName)
{
    tableIter tIt = inmemory_table.lower_bound(make_pair(fileName,0));
    for (; tIt!=inmemory_table.end() && tIt->first.first==fileName; ){
        BufferManagerWrite(*(tIt++->second));
    }
    cout << remove(fileName.c_str()) << " " <<fileName<<endl;
}
