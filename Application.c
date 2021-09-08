#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h>

BOOL ClearConsole(const HANDLE handle)
{
	if (!handle)
	{
		return FALSE;
	}

	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(handle, &info))
	{
		return FALSE;
	}

	DWORD count;
	const DWORD cells = info.dwSize.X * info.dwSize.Y;

	const COORD position = { 0, 0 };
	if (!FillConsoleOutputCharacterW(handle, ' ', cells, position, &count))
	{
		return FALSE;
	}

	if (!FillConsoleOutputAttribute(handle, info.wAttributes, cells, position, &count))
	{
		return FALSE;
	}

	return SetConsoleCursorPosition(handle, position);
}

const CHAR* GetExecutableDirectory(const CHAR** argv)
{
	if (!argv)
	{
		return NULL;
	}

	const CHAR* directory = argv[0];
	const SIZE_T length = strlen(directory);

	const CHAR* exe = strrchr(directory, '\\');

	if (!exe)
	{
		return NULL;
	}

	const SIZE_T exe_length = strlen(exe);

	const SIZE_T size = length - exe_length + 1;
	CHAR* path = (CHAR*) malloc(size + 1);

	if (!path)
	{
		return NULL;
	}

	strncpy(path, directory, size);
	path[size] = '\0';

	return path;
}

BOOL ChangeDirectory(const CHAR* directory)
{
	if (!directory)
	{
		return FALSE;
	}

	return _chdir(directory) == 0;
}

BOOL SetExecutableAsCurrentDirectory(const CHAR** argv)
{
	if (!argv)
	{
		return FALSE;
	}

	const CHAR* directory = GetExecutableDirectory(argv);

	if (!directory)
	{
		return FALSE;
	}

	const BOOL successful = ChangeDirectory(directory);
	free((void*) directory);
	return successful;
}

CHAR** GetImageFiles(const CHAR* path, SIZE_T* count)
{
	if (!path)
	{
		return NULL;
	}

	WIN32_FIND_DATAA info;

	const SIZE_T length = strlen(path) + 1;
	CHAR* duplicate = (CHAR*)malloc(length + 1);

	if (!duplicate)
	{
		return NULL;
	}

	if (!count)
	{
		return NULL;
	}

	strncpy(duplicate, path, length);

	duplicate[length - 1] = '*';
	duplicate[length] = '\0';

	const HANDLE handle = FindFirstFileA(duplicate, &info);

	free(duplicate);

	if (!handle || handle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	CHAR** files = (CHAR**) malloc(100);

	if (!files)
	{
		return NULL;
	}

	SIZE_T i = *count = 0;

	do
	{
		if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}

		if (_stricmp(info.cFileName, ".") == 0 || _stricmp(info.cFileName, "..") == 0)
		{
			continue;
		}

		const CHAR* filename = info.cFileName;
		const CHAR* end = strrchr(filename, '.');
		if (strcmp(end, ".ansi") != 0 && strcmp(end, ".txt") != 0)
		{
			continue;
		}

		if (i % 101 >= 100)
		{
			CHAR** reallocate = (CHAR**) realloc((void*) files, i + 100);

			if (!reallocate)
			{
				*count = i;
				return files;
			}

			files = reallocate;
		}

		files[i++] = _strdup(filename);

	} while (FindNextFileA(handle, &info));

	FindClose(handle);

	*count = i;
	return files;
}

const char* ReadImageFile(const char* filepath)
{
	if (!filepath)
	{
		return NULL;
	}

	FILE* image = fopen(filepath, "r");

	if (!image)
	{
		return NULL;
	}

	fseek(image, 0L, SEEK_END);
	const SIZE_T length = (SIZE_T) _ftelli64(image);

	fseek(image, 0L, SEEK_SET);

	if ((LONG) length <= 0)
	{
		return NULL;
	}

	CHAR* buffer = (CHAR*) malloc(length);

	if (!buffer)
	{
		return NULL;
	}

	fread(buffer, sizeof(CHAR), length, image);
	fclose(image);

	return buffer;
}

COORD GetImageSize(const CHAR* image)
{
	SHORT width = 0;
	SHORT height = 0;

	COORD size = { width, height };

	if (!image)
	{
		return size;
	}

	//Regular expression \x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])

	const SIZE_T length = strlen(image);

	CHAR* duplicate = (CHAR*) calloc(length, sizeof(CHAR));

	if (!duplicate)
	{
		return size;
	}

	for (SIZE_T i = 0, j = 0; i < length;) 
	{
		CHAR character = image[i];
		if (character != 0x1B)
		{
			duplicate[j++] = image[i++];
			continue;
		}

		SIZE_T position = i;
		character = image[++position];

		if (character == '[')
		{
			character = image[++position];

			while (character >= '0' && character <= '?')
			{
				character = image[++position];
			}

			while (character >= ' ' && character <= '/')
			{
				character = image[++position];
			}

			if (character >= '@' && character <= '~')
			{
				i = ++position;
			}

			continue;
		}

		if (character >= '@' && character <= 'Z' || character == '-' || character == '_')
		{
			i = ++position;
			continue;
		}

		duplicate[j++] = character;
	}
	
	while (width < SHRT_MAX && duplicate[width] != '\0')
	{
		if (duplicate[width] == '\n')
		{
			break;
		}

		width++;
	}

	CHAR* token = strtok((CHAR*) duplicate, "\r\n");

	while (token)
	{
		height++;
		token = strtok(NULL, "\r\n");
	}

	size.X = width;
	size.Y = ++height;
	
	free((void*) duplicate);

	return size;
}

BOOL SetConsoleFontSize(const HANDLE handle, const SHORT width, const SHORT height)
{
	CONSOLE_FONT_INFOEX info;
	info.cbSize = sizeof info;
	info.nFont = 0;
	info.dwFontSize.X = width;
	info.dwFontSize.Y = height;
	info.FontFamily = FF_DONTCARE;
	info.FontWeight = FW_NORMAL;
	wcscpy(info.FaceName, L"Consolas");

	return SetCurrentConsoleFontEx(handle, 0, &info);
}

BOOL SetWindowSize(const HANDLE handle, const SHORT width, const SHORT height, const SHORT width_buffer, const SHORT height_buffer)
{
	if (!handle)
	{
		return FALSE;
	}
	
	COORD size;
	size.X = width_buffer;
	size.Y = height_buffer;

	SMALL_RECT rectangle;
	rectangle.Top = 0;
	rectangle.Left = 0;
	rectangle.Bottom = height - 1;
	rectangle.Right = width - 1;

	SetConsoleScreenBufferSize(handle, size);
	return SetConsoleWindowInfo(handle, TRUE, &rectangle);
}

BOOL MoveWindowToCenter(const HWND handle)
{
	if (!handle)
	{
		return FALSE;
	}

	const HWND desktop = GetDesktopWindow();

	if (!desktop)
	{
		return FALSE;
	}

	const SIZE screen = { GetSystemMetrics(0), GetSystemMetrics(1) };
	
	RECT rectangle;
	if (!GetWindowRect(handle, &rectangle))
	{
		return FALSE;
	}

	const INT width = rectangle.right - rectangle.left;
	const INT height = rectangle.bottom - rectangle.top;

	const INT posx = screen.cx / 2 - width / 2;
	const INT posy = screen.cy / 2 - height / 2;

	Sleep(25);
	return SetWindowPos(handle, NULL, posx, posy, width, height, SWP_SHOWWINDOW || SWP_NOSIZE);
}

int main(const INT argc, const CHAR** argv)
{
	if (argc > 0)
	{
		SetExecutableAsCurrentDirectory(argv);
	}

	const HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE); //получаем дескриптор ввода консоли
	const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE); //получаем дескриптор вывода консоли

	if (!handle || handle == INVALID_HANDLE_VALUE) //проверяем валидность дескриптора
	{
		printf("%s", "Console output handle is invalid!");
		exit(1);
	}

	ClearConsole(handle);

	DWORD mode; //объявляем переменную DWORD
	if (!GetConsoleMode(handle, &mode)) //получаем текущий режим консоли на вывод
	{
		printf("%s", "Can't get current console mode!");
		exit(2);
	}

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN; //добавляем необходимые флаги

	if (!SetConsoleMode(handle, mode)) //устанавливаем новый режим терминала на вывод
	{
		printf("%s", "This windows version is not supported!");
		exit(3);
	}

	DWORD input_mode;
	if (GetConsoleMode(input_handle, &input_mode)) //получаем текущий режим консоли на ввод
	{
		input_mode |= ENABLE_EXTENDED_FLAGS;
		input_mode &= ~ENABLE_QUICK_EDIT_MODE;
		SetConsoleMode(input_handle, input_mode); //устанавливаем новый режим терминала на ввод
	}

	const CHAR* filepath;
	if (argc > 1) //проверяем количество аргументов
	{
		filepath = _strdup(argv[1]); //получаем путь к файлу
	}
	else
	{
		const CHAR* directory = GetExecutableDirectory(argv);
		SIZE_T files_count;
		CHAR** images = GetImageFiles(directory, &files_count);

		if (!images || files_count == 0)
		{
			printf("%s", "Files not found");
			exit(4);
		}
		
		SIZE_T selected = 0;
		while (TRUE)
		{
			ClearConsole(handle);

			for (SIZE_T i = 0; i < files_count; i++)
			{
				const CHAR* format = i == selected ? "[*] %s\n" : "[ ] %s\n";
				printf(format, images[i]);
			}

			BOOL arrow = FALSE;

			retry:
			Sleep(25);
			switch (_getch())
			{
			case 224:
				arrow = TRUE;
				goto retry;
			case 'K':
			case 'H':
				if (!arrow)
				{
					goto retry;
				}
			case 'a':
			case 'A':
			case 'w':
			case 'W':
				selected--;
				if (selected == SIZE_MAX)
				{
					selected = files_count - 1;
				}

				break;
			case 'M':
			case 'P':
				if (!arrow)
				{
					goto retry;
				}
			case 's':
			case 'S':
			case 'd':
			case 'D':
				selected++;

				if (selected >= files_count)
				{
					selected = 0;
				}

				break;
			case ' ':
			case '\r':
				ClearConsole(handle);
				filepath = images[selected];
				goto next;
			default:
				goto retry;
			}
		}

		next:

		for (SIZE_T i = 0; i < files_count; i++)
		{
			if (i != selected)
			{
				free((void*) images[i]);
			}
		}

		free((void*) images);
		free((void*) directory);
	}

	if (!filepath)
	{
		printf("%s", "Files not found");
		exit(5);
	}

	SetConsoleTitleA(filepath);

	const CHAR* content = ReadImageFile(filepath); //читаем файл
	
	free((void*) filepath);

	if (!content)
	{
		printf("%s", "Can't read file");
		exit(6);
	}

	SetConsoleFontSize(handle, 1, 1); //устанавливаем размер шрифта консоли равным 1

	const COORD size = GetImageSize(content); //получаем размер изображения

	if (size.X > 0 && size.Y > 0)
	{
		SetWindowSize(handle, size.X, size.Y, size.X, size.Y);
	}

	MoveWindowToCenter(GetConsoleWindow());

	printf("%s", content); //выводим изображение

	free((void*) content); //очищаем данные из файла

	const COORD coordinate = { 0, 0 };
	SetConsoleCursorPosition(handle, coordinate); //перемещаем курсор на начальную позицию

	while (_getch() != ' '){} //ждем нажатия пробела перед завершением программы.
	return 0;
}
