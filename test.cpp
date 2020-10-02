// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"


void ExitTimeout(void* parg)
{
}

void Shutdown(void* parg)
{
    static CCriticalSection cs_Shutdown;
    static bool fTaken;
    bool fFirstThread;
    CRITICAL_BLOCK(cs_Shutdown)
    {
        fFirstThread = !fTaken;
        fTaken = true;
    }
    static bool fExit;
    if (fFirstThread)
    {
        fShutdown = true;
        nTransactionsUpdated++;
        DBFlush(false);
        StopNode();
        DBFlush(true);
        CreateThread(ExitTimeout, NULL);
        Sleep(50);
        printf("Bitcoin exiting\n\n");
        fExit = true;
        exit(0);
    }
    else
    {
        while (!fExit)
            Sleep(500);
        Sleep(100);
        ExitThread(0);
    }
}

bool GetStartOnSystemStartup() { return false; }
void SetStartOnSystemStartup(bool fAutoStart) { }


//////////////////////////////////////////////////////////////////////////////
//
// CMyApp
//

// Define a new application
class CMyApp: public wxApp
{
public:
    wxLocale m_locale;

    CMyApp(){};
    ~CMyApp(){};
    bool OnInit();
    bool OnInit2();
    int OnExit();

    // Hook Initialize so we can start without GUI
    virtual bool Initialize(int& argc, wxChar** argv);

    // 2nd-level exception handling: we get all the exceptions occurring in any
    // event handler here
    virtual bool OnExceptionInMainLoop();

    // 3rd, and final, level exception handling: whenever an unhandled
    // exception is caught, this function is called
    virtual void OnUnhandledException();

    // and now for something different: this function is called in case of a
    // crash (e.g. dereferencing null pointer, division by 0, ...)
    virtual void OnFatalException();
};

IMPLEMENT_APP(CMyApp)

bool CMyApp::Initialize(int& argc, wxChar** argv)
{
    if (argc > 1 && argv[1][0] != '-' && (!fWindows || argv[1][0] != '/') &&
        wxString(argv[1]) != "start")
    {
        fCommandLine = true;
    }
    else if (!fGUI)
    {
        fDaemon = true;
    }
    else
    {
        // wxApp::Initialize will remove environment-specific parameters,
        // so it's too early to call ParseParameters yet
        for (int i = 1; i < argc; i++)
        {
            wxString str = argv[i];
            // haven't decided which argument to use for this yet
            if (str == "-daemon" || str == "-d" || str == "start")
                fDaemon = true;
        }
    }
    return wxApp::Initialize(argc, argv);
}

bool CMyApp::OnInit()
{
    bool fRet = false;
    try
    {
        fRet = OnInit2();
    }
    catch (std::exception& e) {
        PrintException(&e, "OnInit()");
    } catch (...) {
        PrintException(NULL, "OnInit()");
    }
    if (!fRet)
        Shutdown(NULL);
    return fRet;
}

extern int g_isPainting;

bool CMyApp::OnInit2()
{
    //
    // Parameters
    //
    if (fCommandLine)
    {
        int ret = CommandLineRPC(argc, argv);
        exit(ret);
    }

    ParseParameters(argc, argv);
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        wxString strUsage = string() +
          _("Usage:") + "\t\t\t\t\t\t\t\t\t\t\n" +
            "  bitcoin [options]       \t" + "\n" +
            "  bitcoin [command]       \t" + _("Send command to bitcoin running with -server or -daemon\n") +
            "  bitcoin [command] -?    \t" + _("Get help for a command\n") +
            "  bitcoin help            \t" + _("List commands\n") +
          _("Options:\n") +
            "  -gen            \t  " + _("Generate coins\n") +
            "  -gen=0          \t  " + _("Don't generate coins\n") +
            "  -min            \t  " + _("Start minimized\n") +
            "  -datadir=<dir>  \t  " + _("Specify data directory\n") +
            "  -proxy=<ip:port>\t  " + _("Connect through socks4 proxy\n") +
            "  -addnode=<ip>   \t  " + _("Add a node to connect to\n") +
            "  -connect=<ip>   \t  " + _("Connect only to the specified node\n") +
            "  -server         \t  " + _("Accept command line and JSON-RPC commands\n") +
            "  -daemon         \t  " + _("Run in the background as a daemon and accept commands\n") +
            "  -?              \t  " + _("This help message\n");


        if (fWindows && fGUI)
        {
            // Tabs make the columns line up in the message box
            wxMessageBox(strUsage, "Bitcoin", wxOK);
        }
        else
        {
            // Remove tabs
            strUsage.Replace("\t", "");
            fprintf(stderr, "%s", ((string)strUsage).c_str());
        }
        return false;
    }
    ShrinkDebugFile();

    fGenerateBitcoins = true;

    string strErrors;
    strErrors = "";
    int64 nStart;

    printf("Loading addresses...\n");
    nStart = GetTimeMillis();
    if (!LoadAddresses())
        strErrors += _("Error loading addr.dat      \n");
    printf(" addresses   %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading block index...\n");
    nStart = GetTimeMillis();
    if (!LoadBlockIndex())
        strErrors += _("Error loading blkindex.dat      \n");
    printf(" block index %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    printf("Loading wallet...\n");
    nStart = GetTimeMillis();
    bool fFirstRun;
    if (!LoadWallet(fFirstRun))
        strErrors += _("Error loading wallet.dat      \n");
    printf(" wallet      %15"PRI64d"ms\n", GetTimeMillis() - nStart);

    // Add node
    SOCKET socket;
    CAddress addr;
    CNode* pnode = new CNode(socket, addr, false);
    vNodes.push_back(pnode);
    
    BitcoinMiner(false);
}

int CMyApp::OnExit()
{
    Shutdown(NULL);
    return wxApp::OnExit();
}

bool CMyApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnExceptionInMainLoop()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }
    return true;
}

void CMyApp::OnUnhandledException()
{
    // this shows how we may let some exception propagate uncaught
    try
    {
        throw;
    }
    catch (std::exception& e)
    {
        PrintException(&e, "CMyApp::OnUnhandledException()");
        wxLogWarning("Exception %s %s", typeid(e).name(), e.what());
        Sleep(1000);
        throw;
    }
    catch (...)
    {
        PrintException(NULL, "CMyApp::OnUnhandledException()");
        wxLogWarning("Unknown exception");
        Sleep(1000);
        throw;
    }
}

void CMyApp::OnFatalException()
{
    wxMessageBox(_("Program has crashed and will terminate.  "), "Bitcoin", wxOK | wxICON_ERROR);
}
