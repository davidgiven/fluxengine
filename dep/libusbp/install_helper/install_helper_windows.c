/* This file contains special functions that can help install an application
 * that uses USB.  The functions can be used from an MSI custom action or from
 * rundll32. */

#include <windows.h>
#include <devpropdef.h>
#include <msiquery.h>
#include <setupapi.h>
#include <strsafe.h>

#define LIBUSBP_UNUSED(param_name) (void)param_name;

typedef struct install_context
{
    HWND owner;
    MSIHANDLE install;
} install_context;

static void log_message(install_context * context, LPCWSTR message)
{
	if (context->install == 0)
    {
        // The MSI handle is not available so just ignore log messages.
    }
    else
    {
		// Report the log message through MSI, which will put it in the log
		// file.
        MSIHANDLE record = MsiCreateRecord(1);
        MsiRecordSetStringW(record, 0, message);
        MsiProcessMessage(context->install, INSTALLMESSAGE_INFO, record);
        MsiCloseHandle(record);
    }
}

// Adds an error message to the Windows Installer log file and displays it
// to the user in a dialog box with an OK button.
static void error_message(install_context * context, LPCWSTR message)
{
	if (context->install == 0)
	{
		// The MSIHANDLE is not available, so just display a dialog box.
		MessageBoxW(context->owner, message, L"Installation Error", MB_ICONERROR);
	}
	else
	{
		// Report the error through MSI, which will in turn display the dialog
		// box.
	    MSIHANDLE record = MsiCreateRecord(1);
	    MsiRecordSetStringW(record, 0, message);
	    MsiProcessMessage(context->install, INSTALLMESSAGE_ERROR, record);
        MsiCloseHandle(record);
	}
}

/* You might need to call this function after modifying the PATH in order to
 * notify other programs about the change.  This allows a newly launched Command
 * Prompt to see that the PATH has changed and start using it. */
static void broadcast_setting_change_core(install_context * context)
{
	DWORD_PTR result2 = 0;
	LRESULT result = SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0,
        (LPARAM)L"Environment", SMTO_ABORTIFHUNG, 5000, &result2);
	if (result == 0)
	{
		WCHAR message[1024];
		StringCbPrintfW(message, sizeof(message),
            L"SendMessageTimeout failed: Error code 0x%lx.  Result %d",
            GetLastError(), result2);
		error_message(context, message);
	}
}

// Usage: rundll32 libusbp*.dll libusbp_broadcast_setting_change
void __stdcall libusbp_broadcast_setting_changeW(
    HWND owner, HINSTANCE hinst, LPWSTR args, int n)
{
    LIBUSBP_UNUSED(hinst);
    LIBUSBP_UNUSED(args);
    LIBUSBP_UNUSED(n);
    install_context context = {0};
    context.owner = owner;
	broadcast_setting_change_core(&context);
}

// Usage: make a Custom Action in an MSI with this function as the entry point.
UINT __stdcall libusbp_broadcast_setting_change(MSIHANDLE install)
{
    install_context context = {0};
    context.install = install;
    log_message(&context, L"libusbp_broadcast_setting_change: Begin.");
	broadcast_setting_change_core(&context);
    log_message(&context, L"libusbp_broadcast_setting_change: End.");
	return 0;  // Always return success.
}

/* Calls SetupCopyOEMInf to install the specified INF file.  The user may be
 * prompted to accept the driver software, and if everything works then the file
 * will be copied to the C:\Windows\inf directory. */
static void install_inf_core(install_context * context, LPWSTR filename)
{
	BOOL success = SetupCopyOEMInfW(filename, NULL, SPOST_PATH, 0, NULL, 0, NULL, NULL);

	if (!success)
	{
		WCHAR message[1024];

		// NOTE: newlines do not show up in the MSI log, but they do show up in
		// the MSI error dialog box.

		StringCbPrintfW(message, sizeof(message),
            L"There was an error installing the driver file %s.  \n"
			L"You might have to manually install this file by right-clicking it "
            L"and selecting \"Install\".  \n"
			L"Error code 0x%lx.", filename, GetLastError());
		error_message(context, message);
	}
}

// Usage: rundll32 libusbp*.dll libusbp_install_inf path
void __stdcall libusbp_install_infW(HWND owner, HINSTANCE hinst, LPWSTR args, int n)
{
    LIBUSBP_UNUSED(hinst);
    LIBUSBP_UNUSED(args);
    LIBUSBP_UNUSED(n);
    install_context context = {0};
    context.owner = owner;
    install_inf_core(&context, args);
}

// Usage: make a Custom Action in your installer with a "CustomActionData"
// property set equal to the full path of the inf file.
UINT __stdcall libusbp_install_inf(MSIHANDLE install)
{
    install_context context = {0};
    context.install = install;
	broadcast_setting_change_core(&context);

	log_message(&context, L"libusbp_install_inf: Begin.");

	WCHAR message[1024];

	// Get the name of inf file.
	WCHAR filename[1024];
	DWORD length = 1024;
	UINT result = MsiGetPropertyW(install, L"CustomActionData", filename, &length);
	if (result != ERROR_SUCCESS)
	{
		StringCbPrintfW(message, sizeof(message),
            L"libusbp_install_inf: Unable to get filename parameter.  Error code %d.", result);
		error_message(&context, message);
		return 0;  // Return success anyway.
	}

	StringCbPrintfW(message, sizeof(message), L"libusbp_install_inf: filename=%s", filename);
	log_message(&context, message);

	install_inf_core(&context, filename);

	StringCbPrintfW(message, sizeof(message), L"libusbp_install_inf: End. result2=%d", result);
	log_message(&context, message);

    // Always return 0 even if there was an error, because we don't want to roll
    // back the rest of the installation just because this part fails.  The user
    // can either manually install the INF files or try again.
	return 0;
}
