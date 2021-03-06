#include "ForwardsClientListener.h"
#include "BoostPythonSM.h"
#include "ForwardsModule.h"
#include "Util.h"
#include "sdk/smsdk_ext.h"

namespace py = boost::python;

bool ForwardsClientListener::InterceptClientConnect(int clientIndex, char *error, size_t maxlength) {
	py::list args;
	args.append<int>(clientIndex);

	try {
		forwards__ClientConnectForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}

	if(false == forwards_ClientConnect_RejectConnection) {
		return true;
	}

	UTIL_Format(error, maxlength, "%s", forwards_ClientConnect_RejectMessage.c_str());
	RETURN_META_VALUE(MRES_SUPERCEDE, false); // TODO:: WE SHOULD NOT HAVE TO DO THIS!!!
}

void ForwardsClientListener::OnClientConnected(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);

	try {
		forwards__ClientConnectedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnClientPutInServer(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);

	try {
		forwards__ClientPutInServerForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnClientDisconnecting(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);

	try {
		forwards__ClientDisconnectingForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnClientDisconnected(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);

	try {
		forwards__ClientDisconnectedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnClientAuthorized(int clientIndex, const char *authstring) {
	py::list args;
	args.append<int>(clientIndex);
	args.append<std::string>(std::string(authstring));

	try {
		forwards__ClientAuthorizedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnServerActivated(int max_clients) {
	py::list args;
	args.append<int>(max_clients);
	
	try {
		forwards__ServerActivatedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

bool ForwardsClientListener::OnClientPreAdminCheck(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);
	
	try {
		forwards__ClientPreAdminCheckForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}

	return forwards_ClientPreAdminCheck_ReturnValue;
}

void ForwardsClientListener::OnClientPostAdminCheck(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);
	
	try {
		forwards__ClientPostAdminCheckForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}

}

void ForwardsClientListener::OnMaxPlayersChanged(int newvalue) {
	py::list args;
	args.append<int>(newvalue);
	
	try {
		forwards__MaxPlayersChangedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}

void ForwardsClientListener::OnClientSettingsChanged(int clientIndex) {
	py::list args;
	args.append<int>(clientIndex);
	
	try {
		forwards__ClientSettingsChangedForward->Fire(args);
	}
	catch(const py::error_already_set &) {
		PyErr_Print();
	}
}