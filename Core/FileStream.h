#pragma once
#include "Core_Config.h"
#include <Windows.h>
#include <string>
#include <memory>

using namespace std;

typedef HANDLE FileHandle;
#define Invalid_FileHandle INVALID_HANDLE_VALUE //invalid file handle

ENUM_FLAGSEX(FileAdvancedAccess, unsigned long)
/// <summary>
/// advanced file access mode
/// </summary>
enum class FileAdvancedAccess : unsigned long
{
	//
	// Standart Section
	//

	AccessSystemSecurity = 0x1000000,   // AccessSystemAcl access type
	MaximumAllowed = 0x2000000,     // MaximumAllowed access type

	Delete = 0x10000,
	ReadControl = 0x20000,
	WriteDAC = 0x40000,
	WriteOwner = 0x80000,
	Synchronize = 0x100000,

	StandardRightsRequired = 0xF0000,
	StandardRightsRead = ReadControl,
	StandardRightsWrite = ReadControl,
	StandardRightsExecute = ReadControl,
	StandardRightsAll = 0x1F0000,
	SpecificRightsAll = 0xFFFF,

	File_Read_Data = 0x0001,        // file & pipe
	File_List_Directory = 0x0001,       // directory
	File_Write_Data = 0x0002,       // file & pipe
	File_Add_File = 0x0002,         // directory
	File_Append_Data = 0x0004,      // file
	File_Add_SubDirectory = 0x0004,     // directory
	File_Create_Pipe_Instance = 0x0004, // named pipe
	File_Read_EA = 0x0008,          // file & directory
	File_Write_EA = 0x0010,         // file & directory
	File_Execute = 0x0020,          // file
	File_Traverse = 0x0020,         // directory
	File_Delete_Child = 0x0040,     // directory
	File_Read_Attributes = 0x0080,      // all
	File_Write_Attributes = 0x0100,     // all

	//
	// Generic Section
	//

	GenericRead = 0x80000000,
	GenericWrite = 0x40000000,
	GenericExecute = 0x20000000,
	GenericAll = 0x10000000,

	Specific_Rights_All = 0x00FFFF,
	File_All_Access = StandardRightsRequired | Synchronize | 0x1FF,

	File_Generic__Read = StandardRightsRead | File_Read_Data | File_Read_Attributes | File_Read_EA | Synchronize,

	File_Generic__Write = StandardRightsWrite | File_Write_Data | File_Write_Attributes | File_Write_EA | File_Append_Data | Synchronize,

	File_Generic__ReadWrite = ReadControl | File_Read_Data | File_Write_Data | File_Read_Attributes | File_Write_Attributes | File_Read_EA | File_Write_EA | File_Append_Data | Synchronize,

	File_Generic_Execute = StandardRightsExecute | File_Read_Attributes | File_Execute | Synchronize
};


ENUM_OPERATORS(FileAccess)
/// <summary>
/// file access competence
/// </summary>
enum class FileAccess
{
	Read,
	Write,
	All,
};

ENUM_FLAGSEX(FileShare, unsigned long)
/// <summary>
/// file share competence
/// </summary>
enum class FileShare :unsigned long
{
	/// <summary>
	/// 
	/// </summary>
	None = 0x00000000,
	/// <summary>
	/// Enables subsequent open operations on an object to request read access. 
	/// Otherwise, other processes cannot open the object if they request read access. 
	/// If this flag is not specified, but the object has been opened for read access, the function fails.
	/// </summary>
	Read = 0x00000001,
	/// <summary>
	/// Enables subsequent open operations on an object to request write access. 
	/// Otherwise, other processes cannot open the object if they request write access. 
	/// If this flag is not specified, but the object has been opened for write access, the function fails.
	/// </summary>
	Write = 0x00000002,

	/// <summary>
	/// Enables subsequent open and write operations on an object.
	/// </summary>
	ReadWrite = 0x00000003,

	/// <summary>
	/// Enables subsequent open operations on an object to request delete access. 
	/// Otherwise, other processes cannot open the object if they request delete access.
	/// If this flag is not specified, but the object has been opened for delete access, the function fails.
	/// </summary>
	Delete = 0x00000004
};

ENUM_OPERATORS(FileCreationDisposition)
/// <summary>
/// file creation disposition
/// </summary>
enum class FileCreationDisposition
{
	/// <summary>
	/// Creates a new file. The function fails if a specified file exists.
	/// </summary>
	New = 1,
	/// <summary>
	/// Creates a new file, always. 
	/// If a file exists, the function overwrites the file, clears the existing attributes, combines the specified file attributes, 
	/// and flags with FILE_ATTRIBUTE_ARCHIVE, but does not set the security descriptor that the SECURITY_ATTRIBUTES structure specifies.
	/// </summary>
	CreateAlways = 2,
	/// <summary>
	/// Opens a file. The function fails if the file does not exist. 
	/// </summary>
	OpenExisting = 3,
	/// <summary>
	/// Opens a file, always. 
	/// If a file does not exist, the function creates a file as if dwCreationDisposition is CREATE_NEW.
	/// </summary>
	OpenAlways = 4,
	/// <summary>
	/// Opens a file and truncates it so that its size is 0 (zero) bytes. The function fails if the file does not exist.
	/// The calling process must open the file with the GENERIC_WRITE access right. 
	/// </summary>
	TruncateExisting = 5
};

ENUM_OPERATORS(FileMode)
/// <summary>
/// file mode
/// </summary>
enum class FileMode
{
	CreateNew = 1,
	/// <summary>
	/// Creates a new file, always. 
	/// If a file exists, the function overwrites the file, clears the existing attributes, combines the specified file attributes, 
	/// and flags with FILE_ATTRIBUTE_ARCHIVE, but does not set the security descriptor that the SECURITY_ATTRIBUTES structure specifies.
	/// </summary>
	Create = 2,
	/// <summary>
	/// Opens a file. The function fails if the file does not exist. 
	/// </summary>
	Open = 3,
	/// <summary>
	/// Opens a file, always. 
	/// If a file does not exist, the function creates a file as if dwCreationDisposition is CREATE_NEW.
	/// </summary>
	OpenOrCreate = 4,
	/// <summary>
	/// Opens a file and truncates it so that its size is 0 (zero) bytes. The function fails if the file does not exist.
	/// The calling process must open the file with the GENERIC_WRITE access right. 
	/// </summary>
	Truncate = 5,
	/// <summary>
	/// Opens a file, always. 
	/// If a file does not exist, the function creates a file as if dwCreationDisposition is CREATE_NEW.
	/// Then the file will be seeked to the end.
	/// </summary>
	Append = 6
};

ENUM_FLAGSEX(FileAttributes, unsigned long)
/// <summary>
/// file properties
/// </summary>
enum class FileAttributes :unsigned long
{
	None = 0x0000000,
	Readonly = 0x00000001,
	Hidden = 0x00000002,
	System = 0x00000004,
	Directory = 0x00000010,
	Archive = 0x00000020,
	Device = 0x00000040,
	Normal = 0x00000080,
	Temporary = 0x00000100,
	SparseFile = 0x00000200,
	ReparsePoint = 0x00000400,
	Compressed = 0x00000800,
	Offline = 0x00001000,
	NotContentIndexed = 0x00002000,
	Encrypted = 0x00004000,
	Write_Through = 0x80000000,
	Overlapped = 0x40000000,
	NoBuffering = 0x20000000,
	RandomAccess = 0x10000000,
	SequentialScan = 0x08000000,
	DeleteOnClose = 0x04000000,
	BackupSemantics = 0x02000000,
	PosixSemantics = 0x01000000,
	OpenReparsePoint = 0x00200000,
	OpenNoRecall = 0x00100000,
	FirstPipeInstance = 0x00080000
};

/// <summary>
/// file type
/// </summary>
enum class FileType
{
	/// <summary>			
	/// The specified file is a character file, typically an LPT device or a console.
	/// </summary>
	Char =0x0002, 
	/// <summary>			
	/// The specified file is a disk file.
	/// </summary>
	Disk=0x0001,
	/// <summary>			
	/// The specified file is a socket, a named pipe, or an anonymous pipe.
	/// </summary>
	Pipe=0x0003,
	/// <summary>			
	/// Unused.
	/// </summary>
	Remote=0x8000,
	Unknown=0x0000,
};

/// <summary>
/// file stream
/// </summary>
class CORE_API FileStream
{
private:
	FileHandle  _fileHandle;
	bool    _autoClose;
	wstring _path;
	char* _buffer; 
	int _readPos;
	__int64 _readLen;
	__int64 _writePos;
	int _bufferSize;
	bool _canRead;
	bool _canWrite;
	bool _canSeek;
	bool _isPipe;
	unsigned long long _pos;
	__int64 _appendStart;
	FileType _type;

protected:
	/// <summary>
	/// read buffer
	/// </summary>
	/// <param name="buffer">buffer</param>
	/// <param name="size">buffersize</param>
	/// <param name="offset">offset</param>
	/// <param name="count">count</param>
	/// <param name="nothrow">no exception throw</param>			
	unsigned long ReadCore(char* buffer, unsigned long size, unsigned long offset, unsigned long count, bool nothrow = true);

	/// <summary>
	/// write buffer
	/// </summary>
	/// <param name="buffer">buffer</param>
	/// <param name="size">buffer size</param>
	/// <param name="offset">offset</param>
	/// <param name="count">count</param>
	/// <param name="nothrow">no exception throw</param>	
	bool WriteCore(char* buffer, unsigned long size, unsigned long offset, unsigned long count, bool nothrow = true);

	/// <summary>
	/// flush write
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	void FlushWrite(bool nothrow = true);

	/// <summary>
	/// flush read
	/// </summary>
	void FlushRead();

	/// <summary>
	/// seek to position
	/// </summary>
	/// <param name="offset">offset</param>
	/// <param name="origin">position origin</param>
	/// <param name="nothrow">no exception throw</param>
	/// <returns>new position</returns>
	unsigned long long SeekCore(__int64 offset, SeekOrigin origin, bool nothrow = true);

	/// <summary>
	/// set file stream length
	/// </summary>
	/// <param name="newLength">new length</param>
	/// <param name="nothrow">no exception throw</param>
			
	bool SetLengthCore(unsigned long long newLength, bool nothrow = true);

	/// <summary>
	/// flush
	/// </summary>
	void FlushCore();
public:

	/// <summary>
	/// create <see cref="FileStream"/> instance.
	/// </summary>
	FileStream();

	/// <summary>
	/// create <see cref="FileStream"/> instance.
	/// </summary>
	/// <param name="hFile">Windows file handle</param>
	/// <param name="autoClose">auto close after destructor</param>
	FileStream(FileHandle hFile, bool autoClose = true);

	~FileStream();

	operator FileHandle();

	/// <summary>
	/// open file
	/// </summary>
	/// <param name="filePath">file path</param>
	/// <param name="desiredAccess">desired file access </param>
	/// <param name="shareMode">file share competence</param>
	/// <param name="creationDistribution">file creation disposition</param>
	/// <param name="bufferSize">buffer size</param>
	/// <param name="nothrow">no exception throw</param>
	/// <param name="lpSecurityAttributes">WindowsAPI safe security handle</param>
	/// <param name="attributes">file properties</param>
	/// <param name="templateFileHandle">WindowsAPI temporary file handle</param>
	bool Open(const wchar_t* filePath, FileAdvancedAccess desiredAccess, FileShare shareMode, FileCreationDisposition creationDistribution, int bufferSize=4096, bool nothrow = true,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
		FileAttributes attributes = FileAttributes::None,
		FileHandle templateFileHandle = nullptr);

	/// <summary>
	/// open file
	/// </summary>
	/// <param name="filePath">file path</param>
	/// <param name="desiredAccess">desired file access</param>
	/// <param name="shareMode">file share competence</param>
	/// <param name="creationDistribution">file creation disposition</param>
	/// <param name="bufferSize">buffer size</param>
	/// <param name="nothrow">no exception throw</param>
	/// <param name="lpSecurityAttributes">WindowsAPI safe security handle</param>
	/// <param name="attributes">file properties</param>
	/// <param name="templateFileHandle">WindowsAPI temporary file handle</param>
	bool Open(const wchar_t* filePath, FileAccess desiredAccess, FileShare shareMode, FileMode mode, int bufferSize = 4096, bool nothrow = true,
		LPSECURITY_ATTRIBUTES lpSecurityAttributes = nullptr,
		FileAttributes attributes = FileAttributes::None,
		FileHandle templateFileHandle = nullptr);




	/// <summary>
	/// check read access
	/// </summary>
	bool CanRead() const;

	/// <summary>
	/// check write access
	/// </summary>
	bool CanWrite() const;

	/// <summary>
	/// check seek access
	/// </summary>
	bool CanSeek() const;

	/// <summary>
	/// check open status
	/// </summary>
	bool IsOpen() const;

	/// <summary>
	/// check close status
	/// </summary>
	bool IsClosed() const; 

	/// <summary>
	/// attach WindowsAPI file handle
	/// </summary>
	/// <param name="hFile">WindowsAPI file handle</param>
	/// <param name="autoClose">auto close</param>
	void Attach(FileHandle hFile, bool autoClose = true);

	/// <summary>
	/// detach WindowsAPI file handle
	/// </summary>
	void Detach();

	/// <summary>
	/// close file stream
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	void Close(bool nothrow = true);

	/// <summary>
	/// close file stream and delete the disk file
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	void CloseAndDelete(bool nothrow = true);

	/// <summary>
	/// write buffer
	/// </summary>
	/// <param name="buffer">buffer</param>
	/// <param name="size">buffer size</param>
	/// <param name="offset">offset</param>
	/// <param name="count">count</param>
	/// <param name="nothrow">no exception throw</param>			
	bool Write(char* buffer, unsigned long size, unsigned long offset, unsigned long count, bool nothrow = true);

	/// <summary>
	/// write a byte
	/// </summary>
	/// <param name="byte">byte</param>
	/// <param name="nothrow">no exception throw</param>			
	bool WriteByte(int byte, bool nothrow = true);

	/// <summary>
	/// read buffer
	/// </summary>
	/// <param name="buffer">buffer</param>
	/// <param name="size">buffer size</param>
	/// <param name="offset">offset</param>
	/// <param name="count">count</param>
	/// <param name="nothrow">no exception throw</param>
	unsigned long Read(char* buffer, unsigned long size, unsigned long offset, unsigned long count, bool nothrow = true);

	template<typename T>
	bool Read(T& ret, bool nothrow = true)
	{
		return (Read((char*)&ret, sizeof(T), 0, sizeof(T), nothrow)== sizeof(T));
	}

	bool ReadString(std::wstring& ret, bool nothrow = true);

	/// <summary>
	/// read next byte
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	/// <returns>single byte</returns>
	int ReadByte(bool nothrow = true);

	/// <summary>
	/// seek to new position
	/// </summary>
	/// <param name="offset">offset</param>
	/// <param name="origin">position origin</param>
	/// <param name="nothrow">no exception throw</param>
	/// <returns>new position</returns>
	unsigned long long Seek(__int64 offset, SeekOrigin origin, bool nothrow = true);

	/// <summary>
	/// seek to end
	/// </summary>
	/// <returns>new position</returns>
	unsigned long long SeekToEnd();

	/// <summary>
	/// seek back to begin
	/// </summary>
	void SeekToBegin();

	/// <summary>
	/// set file stream length
	/// </summary>
	/// <param name="newLength">new length</param>
	/// <param name="nothrow">no exception throw</param>			
	bool SetLength(unsigned long long newLength, bool nothrow = true);

	/// <summary>
	/// get file stream length
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	/// <returns>file stream length</returns>
	unsigned long long GetLength(bool nothrow = true) const;

	/// <summary>
	/// get file stream position
	/// </summary>
	/// <param name="nothrow">no exception throw</param>		
	unsigned long long GetPosition(bool nothrow = true) const; 

	/// <summary>
	/// set file stream position
	/// </summary>
	/// <param name="newPos">new position</param>
	/// <param name="nothrow">no exception throw</param>			
	bool SetPosition(unsigned long long newPos, bool nothrow=true);

	/// <summary>
	/// flush
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	bool Flush(bool nothrow = true);

	/// <summary>
	/// flush
	/// </summary>
	/// <param name="flushToDisk">flush to disk</param>
	/// <param name="nothrow">no exception throw</param>
	virtual bool Flush(bool flushToDisk, bool nothrow = true);

	/// <summary>
	/// copy file stream
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	/// <returns>new instance of file stream</returns>
	FileStream* Duplicate(bool nothrow = true) const;

	/// <summary>
	/// abort file stream
	/// </summary>
	void Abort();

	/// <summary>
	/// region lock
	/// </summary>
	/// <param name="lPos">position</param>
	/// <param name="lCount">length</param>
	/// <param name="nothrow">no exception throw</param>
	bool LockRange(const unsigned long long& lPos, const unsigned long long& lCount, bool nothrow = true);

	/// <summary>
	/// region lock
	/// </summary>
	/// <param name="lPos">position</param>
	/// <param name="lCount">length</param>
	/// <param name="nothrow">no exception throw</param>		
	bool UnlockRange(const unsigned long long& lPos, const unsigned long long& lCount, bool nothrow = true);

	/// <summary>
	/// get file type
	/// </summary>
	/// <param name="nothrow">no exception throw</param>
	/// <returns> file type</returns>
	FileType GetFileType(bool nothrow = true);
};

typedef std::shared_ptr<FileStream> FileStreamPtr;
