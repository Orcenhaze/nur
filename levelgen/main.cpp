
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

#define ORH_STATIC
#define ORH_IMPLEMENTATION
#include "orh.h"

GLOBAL OS_State global_os;
GLOBAL char *path_to_levels_wild = "../data/levels/*.nlf";
GLOBAL char *output_path         = "../src/levels.h";
GLOBAL char *buildbat_path       = "../src/";

FUNCTION void* win32_reserve(u64 size)
{
    void *memory = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return memory;
}
FUNCTION void  win32_release(void *memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}
FUNCTION void  win32_commit(void *memory, u64 size)
{
    VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
}
FUNCTION void  win32_decommit(void *memory, u64 size)
{
    VirtualFree(memory, size, MEM_DECOMMIT);
}
FUNCTION void win32_print_to_console(String8 text)
{
    printf("%s", (char*)text.data);
}

////////////////////////////////
////////////////////////////////

struct File_Group_Names
{
    s32 file_count;
    String8 file_names[256];
};

FUNCTION File_Group_Names load_file_names(char *path)
{
    File_Group_Names result = {};
    s32 file_count = 0;
    
    WIN32_FIND_DATA find_data;
    HANDLE find_handle = FindFirstFile(path, &find_data); 
    while(find_handle != INVALID_HANDLE_VALUE)
    {
        char *src = find_data.cFileName;
        result.file_names[file_count++] = string_copy(string(src));
        
        if(!FindNextFile(find_handle, &find_data)) break;
    }
    
    result.file_count = file_count;
    
    return result;
}

FUNCTION b32 write_entire_file(char *path, String8 data)
{
    b32 result = false;
    
    HANDLE file_handle = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        print("OS Error: write_entire_file() INVALID_HANDLE_VALUE!\n");
    }
    
    DWORD bytes_written;
    if (WriteFile(file_handle, data.data, (DWORD)data.count, &bytes_written, 0) && (bytes_written == data.count)) {
        result = true;
    } else {
        print("OS Error: write_entire_file() WriteFile() failed!\n");
    }
    
    CloseHandle(file_handle);
    
    return result;
}

FUNCTION void strip_extension(String8 *s)
{
    for (s32 i = 0; i < s->count; i++) {
        if (s->data[i] == '.') {
            s->count   = i;
            s->data[i] = 0;
            break;
        }
    }
}

FUNCTION b32 write_level_names(File_Group_Names level_names)
{
    String_Builder sb = sb_init();
    defer(sb_free(&sb));
    
    sb_appendf(&sb, 
               "////////////////////////////////////////////////////////////////\n"
               "// This file is generated by levelgen/main.exe\n"
               "//\n\n");
    
    sb_appendf(&sb, "GLOBAL String8 level_names[] = \n{\n");
    sb_appendf(&sb, "S8LIT(\"invalid_level\"),\n");
    for (s32 i = 0; i < level_names.file_count; i++) {
        strip_extension(&level_names.file_names[i]);
        sb_appendf(&sb, "S8LIT(\"%S\"),\n", level_names.file_names[i]);
    }
    sb_appendf(&sb, "};");
    
    String8 out = sb_to_string(&sb);
    return write_entire_file(output_path, out);
}

int main()
{
    //
    // Initialize os.
    {
        os = &global_os;
        
        // Functions.
        global_os.reserve          = win32_reserve;
        global_os.release          = win32_release;
        global_os.commit           = win32_commit;
        global_os.decommit         = win32_decommit;
        global_os.print_to_console = win32_print_to_console;
        
        // Arenas.
        global_os.permanent_arena  = arena_init();
    }
    
    File_Group_Names level_names = load_file_names(path_to_levels_wild);
    b32 success = write_level_names(level_names);
    if (success) print("Successfully written levels.h!\n");
    else         print("Couldn't write to levels.h!\n");
    
    
    if (_chdir(buildbat_path) != 0) {
        print("Failed to change the working directory.\n");
        return 1;
    }
    
    s32 err = system("build.bat");
    if (err) print("Couldn't run build.bat! Error %d\n", err);
    else     print("Successfully ran build.bat!\n");
    
    print("Exiting...\n");
    getchar();
    
    return 0;
}