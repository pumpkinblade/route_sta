#include "LefDefReader.hpp"
#include "utils.hpp"
#include <math.h>
#include <sta/Sta.hh>
#include <tcl.h>

using namespace sta;

int read_lefdef_cmd(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *CONST objv[]) {
  if (objc != 3) {
    Tcl_WrongNumArgs(interp, 2, objv, "Usage : read_lefdef lef_file def_file");
    return TCL_ERROR;
  }
  const char *lef_file = Tcl_GetStringFromObj(objv[1], nullptr);
  const char *def_file = Tcl_GetStringFromObj(objv[2], nullptr);

  bool ok = readLefDefFile(lef_file, def_file, Sta::sta()->networkReader());
  if (ok)
    return TCL_OK;
  else
    return TCL_ERROR;
}

int Route_Init(Tcl_Interp *interp) {
  Tcl_CreateObjCommand(interp, "read_lefdef", read_lefdef_cmd, (ClientData)NULL,
                       (Tcl_CmdDeleteProc *)NULL);
  return TCL_OK;
}
