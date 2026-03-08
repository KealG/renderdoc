import importlib
import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', 'util')))
import rdoc_brand as brand

rd = importlib.import_module(brand.PY_CORE_MODULE_NAME)

rd.InitialiseReplay(rd.GlobalEnvironment(), [])

# Open a capture file handle
cap = rd.OpenCaptureFile()

# Open a particular file - see also OpenBuffer to load from memory
result = cap.OpenFile('test.rdc', '', None)

# Make sure the file opened successfully
if result != rd.ResultCode.Succeeded:
    raise RuntimeError("Couldn't open file: " + str(result))

# Make sure we can replay
if not cap.LocalReplaySupport():
    raise RuntimeError("Capture cannot be replayed")

# Initialise the replay
result,controller = cap.OpenCapture(rd.ReplayOptions(), None)

if result != rd.ResultCode.Succeeded:
    raise RuntimeError("Couldn't initialise replay: " + str(result))

# Now we can use the controller!
print("%d top-level actions" % len(controller.GetRootActions()))

controller.Shutdown()

cap.Shutdown()

rd.ShutdownReplay()
