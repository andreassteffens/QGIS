/***************************************************************************
                        sb_mapserver.cpp
 A server application supporting WMS / WFS / WCS
                      --------------------
  begin                : May 05, 2020
  copyright            : (C) 2020 by Andreas Steffens
  email                : a dot steffes at gds dash team dot de
***************************************************************************/

// macros

#define SAFE_RELEASE(x)					do { if(x) { (x)->Release(); (x) = NULL; } } while(0)
#define SAFE_DELETE(x)					do { if(x) { delete (x); (x) = NULL; } } while(0)
#define SAFE_DELETE_ARRAY(x)			do { if(x) { delete[] (x); (x) = NULL; } } while(0)
#define SAFE_FREE(x)					do { if(x) { free(x); (x) = NULL; } } while(0)
#define SAFE_CLOSE_HANDLE(x)			do { if(x) { CloseHandle((x)); (x) = NULL; } } while(0)
#define SAFE_DELETE_OBJECT(x)			do { if(x) { DeleteObject((x)); (x) = NULL; } } while(0)

// ---------------------------------------------------------------------

//for CMAKE_INSTALL_PREFIX
#include "qgsconfig.h"
#include "qgsserver.h"
#include "qgsbufferserverrequest.h"
#include "qgsbufferserverresponse.h"
#include "qgsapplication.h"
#include "qgsmessagelog.h"

#include <QFontDatabase>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QCommandLineParser>
#include <QObject>
#include <iostream>
#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <process.h>
#include <signal.h>
#include <time.h>
#include <string>


#ifndef Q_OS_WIN
#include <csignal>
#endif

#include <string>
#include <chrono>

///@cond PRIVATE

// ---------------------------------------------------------------------

// constants

#define	MAX_STRING_LENGTH			512
#define REQUEST_BUFFER_INITIAL_SIZE	65536
#define STDIN_TIMEOUT				10000

// ---------------------------------------------------------------------

// function forward declarations

void	Debug(const char* pszFormat, ...);
DWORD	ReadNamedPipe(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead);
DWORD	ReadStdInput(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead);
void	EnsureBufferSize(BYTE **pBuffer, UINT *pnCurrentSize, UINT nRequestedSize);

// ---------------------------------------------------------------------

// byte-aligned data structures

#pragma pack(push)

#pragma pack(1) 

typedef struct tagRequestInfo
{
	UINT nUrlLength;
	UINT nRequestMethod;
	UINT nHeaderCount;
	UINT arrHeaderKeySizes[32];
	UINT arrHeaderValueSizes[32];
	UINT nContentSize;
} RequestInfo;

typedef struct tagResponseInfo
{
	UINT nResponseSize;
	UINT nStatusCode;
	UINT nHeaderCount;
	UINT arrHeaderKeySizes[32];
	UINT arrHeaderValueSizes[32];
	UINT nContentSize;
} ResponseInfo;

#pragma pack(pop) 

// ---------------------------------------------------------------------

// main entry point

int main(int iArgCount, char** ppszArgs)
{
	// Test if the environ variable DISPLAY is defined
	// if it's not, the server is running in offscreen mode
	// Qt supports using various QPA (Qt Platform Abstraction) back ends
	// for rendering. You can specify the back end to use with the environment
	// variable QT_QPA_PLATFORM when invoking a Qt-based application.
	// Available platform plugins are: directfbegl, directfb, eglfs, linuxfb,
	// minimal, minimalegl, offscreen, wayland-egl, wayland, xcb.
	// https://www.ics.com/blog/qt-tips-and-tricks-part-1
	// http://doc.qt.io/qt-5/qpa.html
	const QString display{ qgetenv("DISPLAY") };
	bool withDisplay = true;
	if (display.isEmpty())
	{
		withDisplay = false;
		qputenv("QT_QPA_PLATFORM", "offscreen");
	}

	int argc = 0;

	// since version 3.0 QgsServer now needs a qApp so initialize QgsApplication
	QgsApplication app(argc, NULL, withDisplay, QString(), QStringLiteral("QGIS Development Server"));

	QCoreApplication::setOrganizationName(QgsApplication::QGIS_ORGANIZATION_NAME);
	QCoreApplication::setOrganizationDomain(QgsApplication::QGIS_ORGANIZATION_DOMAIN);
	QCoreApplication::setApplicationName("Atapa GmbH EnderGI Server");
	QCoreApplication::setApplicationVersion("1.0");

	if (!withDisplay)
	{
		QgsMessageLog::logMessage("DISPLAY environment variable is not set, running in offscreen mode, all printing capabilities will not be available.\n"
			"Consider installing an X server like 'xvfb' and export DISPLAY to the actual display value.", "Server", Qgis::Warning);
	}

	static const QMap<int, QString> knownStatuses
	{
		{ 200, QStringLiteral("OK") },
		{ 201, QStringLiteral("Created") },
		{ 202, QStringLiteral("Accepted") },
		{ 204, QStringLiteral("No Content") },
		{ 301, QStringLiteral("Moved Permanently") },
		{ 302, QStringLiteral("Moved Temporarily") },
		{ 304, QStringLiteral("Not Modified") },
		{ 400, QStringLiteral("Bad Request") },
		{ 401, QStringLiteral("Unauthorized") },
		{ 403, QStringLiteral("Forbidden") },
		{ 404, QStringLiteral("Not Found") },
		{ 500, QStringLiteral("Internal Server Error") },
		{ 501, QStringLiteral("Not Implemented") },
		{ 502, QStringLiteral("Bad Gateway") },
		{ 503, QStringLiteral("Service Unavailable") }
	};

	QgsServer server;

#ifdef HAVE_SERVER_PYTHON_PLUGINS
	server.initPython();
#endif

	QgsMessageLog::logMessage(QStringLiteral("###################### Server started! ######################"), QStringLiteral("Server"), Qgis::Critical);

	UINT nRequestBufferSize = 0;
	BYTE *pRequestBuffer = NULL;
	bool bTerminate = false;

	EnsureBufferSize(&pRequestBuffer, &nRequestBufferSize, REQUEST_BUFFER_INITIAL_SIZE);

	try
	{
		if (iArgCount < 2)
		{
			Debug("Invalid argument count");
			return -1;
		}

		char szPipeName[MAX_STRING_LENGTH];
		strcpy(szPipeName, ppszArgs[1]);

		HANDLE hStdError = GetStdHandle(STD_ERROR_HANDLE);
		if (hStdError == INVALID_HANDLE_VALUE)
		{
			Debug("Failed to acquire standard error handle");
			return -1;
		}

		HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
		if (hStdIn == INVALID_HANDLE_VALUE)
		{
			Debug("Failed to acquire standard input handle");
			return -1;
		}

		DWORD dwStdInMode;
		if (GetConsoleMode(hStdIn, &dwStdInMode))
		{
			dwStdInMode = dwStdInMode ^ ENABLE_MOUSE_INPUT ^ ENABLE_WINDOW_INPUT;
			if (SetConsoleMode(hStdIn, dwStdInMode))
				FlushConsoleInputBuffer(hStdIn);
		}

		HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (hStdOut == INVALID_HANDLE_VALUE)
		{
			Debug("Failed to acquire standard output handle");
			return -1;
		}

		HANDLE hPipe = CreateNamedPipeA(szPipeName, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 8192, 8192, 0, NULL);
		if (hPipe == INVALID_HANDLE_VALUE)
		{
			Debug("Failed to create pipe at %s", szPipeName);
			return -1;
		}

		if (ConnectNamedPipe(hPipe, NULL) == FALSE)
		{
			Debug("Named pipe received no connections");
			return -1;
		}

		try
		{
			while (!bTerminate)
			{
				RequestInfo request;
				UINT nRequestInfoSize = sizeof(request);
				ZeroMemory(&request, nRequestInfoSize);

				DWORD dwRead = 0;
				DWORD dwWritten = 0;

				dwRead = ReadNamedPipe(hPipe, (LPVOID)&request, nRequestInfoSize);
				if (dwRead == 0)
				{
					DWORD dwError = GetLastError();
					
					bTerminate = false;

					switch (dwError)
					{
						case ERROR_BROKEN_PIPE:
							Debug("Pipe has been closed. Application terminating...");
							bTerminate = true;
							break;
						default:
							Debug("Could not read request from pipe: %d", dwError);
							continue;
							break;
					}
				}

				if (bTerminate)
				{
					FlushFileBuffers(hStdOut);
					continue;
				}

				if (dwRead != nRequestInfoSize)
				{
					Debug("Read only %d bytes from pipe ... expected %d bytes", dwRead, nRequestInfoSize);
					
					FlushFileBuffers(hStdOut);
					continue;
				}
				
				UINT nRequestHeaderSize = request.nUrlLength;
				for (UINT nHeader = 0; nHeader < request.nHeaderCount; nHeader++)
				{
					nRequestHeaderSize += request.arrHeaderKeySizes[nHeader];
					nRequestHeaderSize += request.arrHeaderValueSizes[nHeader];
				}

				EnsureBufferSize(&pRequestBuffer, &nRequestBufferSize, nRequestHeaderSize + request.nContentSize);
				ZeroMemory(pRequestBuffer, nRequestBufferSize);

				dwRead = ReadStdInput(hStdIn, (LPVOID)pRequestBuffer, nRequestHeaderSize);
				if (dwRead == 0)
				{
					Debug("Failed to read request header data from stdin");

					FlushFileBuffers(hStdOut);
					continue;
				}

				UINT nOffset = 0;
				QString strUrl = QString::fromUtf8(reinterpret_cast<char const*>(pRequestBuffer), request.nUrlLength);
				nOffset += request.nUrlLength;

				QgsServerRequest::Method method = QgsServerRequest::Method::GetMethod;
				if (request.nRequestMethod == 0)
					method = QgsServerRequest::Method::GetMethod;
				else if (request.nRequestMethod == 1)
					method = QgsServerRequest::Method::PostMethod;
				else if (request.nRequestMethod == 2)
					method = QgsServerRequest::Method::HeadMethod;
				else if (request.nRequestMethod == 3)
					method = QgsServerRequest::Method::PutMethod;
				else if (request.nRequestMethod == 8)
					method = QgsServerRequest::Method::PatchMethod;
				else if (request.nRequestMethod == 5)
					method = QgsServerRequest::Method::DeleteMethod;
				
				QgsBufferServerRequest::Headers headers;
				for (UINT nHeader = 0; nHeader < request.nHeaderCount; nHeader++)
				{
					QString strKey = QString::fromUtf8(reinterpret_cast<char const*>(pRequestBuffer) + nOffset, request.arrHeaderKeySizes[nHeader]);
					nOffset += request.arrHeaderKeySizes[nHeader];

					Debug("%s", strKey.toStdString().data());
					
					QString strValue = QString::fromUtf8(reinterpret_cast<char const*>(pRequestBuffer) + nOffset, request.arrHeaderValueSizes[nHeader]);
					nOffset += request.arrHeaderValueSizes[nHeader];

					Debug("%s", strValue.toStdString().data());

					headers.insert(strKey, strValue);
				}

				QByteArray arrData = nullptr;
				if (request.nContentSize > 0)
				{
					dwRead = ReadStdInput(hStdIn, (LPVOID)pRequestBuffer, request.nContentSize);
					if (dwRead == 0)
					{
						Debug("Failed to read request content data from stdin");

						FlushFileBuffers(hStdOut);
						continue;
					}
					
					arrData = QByteArray::fromRawData((const char*)pRequestBuffer, request.nContentSize);
				}

				QgsBufferServerRequest qgsRequest{ QUrl(strUrl), method, headers, &arrData };
				QgsBufferServerResponse qgsResponse;

				server.handleRequest(qgsRequest, qgsResponse);

				ResponseInfo response;
				UINT nResponseSize = sizeof(response);
				ZeroMemory((void*)&response, nResponseSize);

				response.nStatusCode = qgsResponse.statusCode();
				response.nHeaderCount = qgsResponse.headers().count();

				QByteArray arrBody = qgsResponse.body();
				response.nContentSize = arrBody.size();

				QString strHeaders = "";
				int iHeader = 0;
				QMap<QString, QString>::const_iterator iterHeaders = qgsResponse.headers().constBegin();
				for (; iterHeaders != qgsResponse.headers().constEnd(); iterHeaders++)
				{
					QString strKey = iterHeaders.key();
					QString strValue = iterHeaders.value();

					response.arrHeaderKeySizes[iHeader] = strKey.length();
					response.arrHeaderValueSizes[iHeader] = strValue.length();

					strHeaders += strKey + strValue;

					iHeader++;
				}

				if (WriteFile(hStdOut, &response, nResponseSize, &dwWritten, NULL) == FALSE)
				{
					Debug("Failed to write response info");
					FlushFileBuffers(hStdOut);
					continue;
				}

				QByteArray arrHeaders = strHeaders.toLatin1();
				if (WriteFile(hStdOut, arrHeaders.data(), strHeaders.length(), &dwWritten, NULL) == FALSE)
				{
					Debug("Failed to write response headers");

					FlushFileBuffers(hStdOut);
					continue;
				}

				if (response.nContentSize)
				{
					if (WriteFile(hStdOut, arrBody.data(), response.nContentSize, &dwWritten, NULL) == FALSE)
						Debug("Failed to write response data to stdout");
				}

				FlushFileBuffers(hStdOut);
			}
		}
		catch (QgsServerException &ex)
		{
			QString format;
			QgsMessageLog::logMessage(ex.formatResponse(format), QStringLiteral("Server"), Qgis::Critical);
		}
		catch (QgsException &ex)
		{
			QgsMessageLog::logMessage(QStringLiteral("QgsException: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
		}
		catch (const std::runtime_error& ex)
		{
			Debug(ex.what());
			QgsMessageLog::logMessage(QStringLiteral("RuntimeError: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
		}
		catch (const std::exception& ex)
		{
			Debug(ex.what());
			QgsMessageLog::logMessage(QStringLiteral("Exception: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
		}
		catch (...)
		{
			Debug("Unknown exception in sbWeb.GIS31.QGIS.ServerTestClient:main_1");
		}

		std::cout << "END" << std::endl;

		DisconnectNamedPipe(hPipe);

		if (hPipe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hPipe);
			hPipe = INVALID_HANDLE_VALUE;
		}

		if (hStdError != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hStdError);
			hStdError = INVALID_HANDLE_VALUE;
		}

		if (hStdIn != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hStdIn);
			hStdIn = INVALID_HANDLE_VALUE;
		}

		if (hStdOut != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hStdOut);
			hStdOut = INVALID_HANDLE_VALUE;
		}
	}
	catch (QgsServerException &ex)
	{
		QString format;
		QgsMessageLog::logMessage(ex.formatResponse(format), QStringLiteral("Server"), Qgis::Critical);
	}
	catch (QgsException &ex)
	{
		QgsMessageLog::logMessage(QStringLiteral("QgsException: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
	}
	catch (const std::runtime_error& ex)
	{
		Debug(ex.what());
		QgsMessageLog::logMessage(QStringLiteral("RuntimeError: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
	}
	catch (const std::exception& ex)
	{
		Debug(ex.what());
		QgsMessageLog::logMessage(QStringLiteral("Exception: %1").arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
	}
	catch (...)
	{
		Debug("Unknown exception in sbWeb.GIS31.QGIS.ServerTestClient:main_2");
		QgsMessageLog::logMessage(QStringLiteral("Unknown exception in sbWeb.GIS31.QGIS.ServerTestClient:main_2"), QStringLiteral("Server"), Qgis::Critical);
	}

	try
	{
		SAFE_FREE(pRequestBuffer);
	}
	catch (...)
	{
		Debug("Unknown exception in sbWeb.GIS31.QGIS.ServerTestClient:main_3");
	}

	// Exit handlers
#ifndef Q_OS_WIN

	auto exitHandler = [](int signal)
	{
		std::cout << QStringLiteral("Signal %1 received: quitting").arg(signal).toStdString() << std::endl;
		qApp->quit();
	};

	signal(SIGTERM, exitHandler);
	signal(SIGABRT, exitHandler);
	signal(SIGINT, exitHandler);
	signal(SIGPIPE, [](int)
	{
		std::cerr << QStringLiteral("Signal SIGPIPE received: ignoring").toStdString() << std::endl;
	});

#endif

	app.exitQgis();

	return 0;
}

// ---------------------------------------------------------------------

// function implementations

DWORD ReadNamedPipe(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead)
{
	DWORD dwRead = 0;

	if (ReadFile(hFile, lpBuffer, dwBytesToRead, &dwRead, NULL) == FALSE)
		return 0;

	if (dwRead < dwBytesToRead)
	{
		for (int i = 1; (i < 6) && (dwRead < dwBytesToRead); i++)
		{
			DWORD dwRes = WaitForSingleObject(hFile, i * 10);
			if (dwRes == WAIT_OBJECT_0)
			{
				DWORD dwBytesReadNext;
				DWORD dwBytesToReadNext = dwBytesToRead - dwRead;

				if (ReadFile(hFile, (void*)((BYTE*)lpBuffer + dwRead), dwBytesToReadNext, &dwBytesReadNext, NULL) == TRUE)
					dwRead += dwBytesReadNext;
			}
			else
				Debug("Error waiting for pipe input: %d", dwRes);
		}
	}

	return dwRead;
}

DWORD ReadStdInput(HANDLE hFile, LPVOID lpBuffer, DWORD dwBytesToRead)
{
	DWORD dwRead = 0;

	DWORD dwRes = WaitForSingleObject(hFile, STDIN_TIMEOUT);
	if (dwRes == WAIT_OBJECT_0)
	{
		if (ReadFile(hFile, lpBuffer, dwBytesToRead, &dwRead, NULL) == FALSE)
			return 0;

		if (dwRead < dwBytesToRead)
		{
			for (int i = 1; (i < 6) && (dwRead < dwBytesToRead); i++)
			{
				dwRes = WaitForSingleObject(hFile, i * 10);
				if (dwRes == WAIT_OBJECT_0)
				{
					DWORD dwBytesReadNext;
					DWORD dwBytesToReadNext = dwBytesToRead - dwRead;

					if (ReadFile(hFile, (void*)((BYTE*)lpBuffer + dwRead), dwBytesToReadNext, &dwBytesReadNext, NULL) == TRUE)
						dwRead += dwBytesReadNext;
				}
				else
					Debug("Error waiting for std input: %d", dwRes);
			}
		}
	}
	else
		Debug("Error waiting for std input: %d", dwRes);

	return dwRead;
}

void EnsureBufferSize(BYTE** pBuffer, UINT *pnCurrentSize, UINT nRequestedSize)
{
	if (*pnCurrentSize >= nRequestedSize && *pBuffer != NULL)
		return;

	SAFE_FREE(*pBuffer);
	*pBuffer = (BYTE*)calloc(nRequestedSize, sizeof(char));

	*pnCurrentSize = nRequestedSize;
}

void Debug(const char* pszFormat, ...)
{
	char szBuffer[MAX_STRING_LENGTH];
	ZeroMemory(szBuffer, MAX_STRING_LENGTH * sizeof(char));

	try
	{
		va_list args;
		va_start(args, pszFormat);
		vsprintf(szBuffer, pszFormat, args);
		va_end(args);

		if (IsDebuggerPresent() || true)
		{
			OutputDebugStringA(szBuffer);
			OutputDebugStringA("\n");

			time_t rawtime;
			struct tm* timeinfo;
			char szTimeBuffer[256];

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(szTimeBuffer, 256, "%d-%m-%Y %I:%M:%S", timeinfo);

			char szOutputBuffer[MAX_STRING_LENGTH];
			sprintf(szOutputBuffer, "[%d | %s] %s", GetCurrentProcessId(), szTimeBuffer, szBuffer);

			char szFile[MAX_STRING_LENGTH];
			strcpy(szFile, "error_log.txt");
			FILE* pf = fopen(szFile, "a");
			if (pf)
			{
				fprintf(pf, "%s\n", szOutputBuffer);
				fclose(pf);
			}

			std::cerr << szOutputBuffer << std::endl;
		}
	}
	catch (...)
	{
		// nothing to be done here for now
	}
}

/// @endcond
