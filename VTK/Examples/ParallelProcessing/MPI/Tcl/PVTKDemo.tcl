# This example demonstrates the use of pvtk.
# pvtk is a slightly different version of wish specially modified
# to be used with MPI. It allows Tcl scripts to be interpreted in
# parallel.
#
# In this example, a synthetic source (vtkRTAnalyticSource) generates 
# image data (only one piece of the data is generated on each processor).
# Next, an isosurface (vtkContourFilter) is generated by each process. 
# The resulting polygonal data is then rendered and then composited 
# (vtkTreeComposite) and displayed on the first process (all processes 
# will create render windows but only the render window of the root 
# process will contain the composited image).

package require vtktcl_interactor

# Create a controller. At this point, MPI has already been initialized
# therefore there is no need to call Initialize.
vtkMPIController controller
# Get the local process id to be used later
set myId [controller GetLocalProcessId]

# The "global" extent of the source is (2*EXTENT+1)*(2*EXTENT+1)*(2*EXTENT+1)
# Each processor will generate one piece of this extent.
set EXTENT 20

# This is a synthetic source. The equation is:
# Maximum*Gaussian*XMag*sin(XFreq*x)*sin(YFreq*y)*cos(ZFreq*z)
vtkRTAnalyticSource source1
source1 SetWholeExtent [expr -1*$EXTENT] $EXTENT  [expr -1*$EXTENT] $EXTENT \
	[expr -1*$EXTENT] $EXTENT 
source1 SetCenter 0 0 0
source1 SetStandardDeviation 0.5 
source1 SetMaximum  255.0 
source1 SetXFreq 60 
source1 SetXMag 10 
source1 SetYFreq 30 
source1 SetYMag 18 
source1 SetZFreq 40 
source1 SetZMag 5 
eval [source1 GetOutput] SetSpacing [expr 2.0/$EXTENT] \
	[expr 2.0/$EXTENT] [expr 2.0/$EXTENT]

# Generate an isosurface
vtkContourFilter cf
cf SetInput [source1 GetOutput]
cf SetNumberOfContours 1
cf SetValue 0 220

vtkElevationFilter  elev
elev SetInput [cf GetOutput]
elev SetScalarRange $myId [expr $myId + 0.001]

# Create the rendering part of the pipeline
vtkPolyDataMapper mapper
mapper SetInput [elev GetOutput]
mapper SetScalarRange 0 [expr [controller GetNumberOfProcesses]]
vtkActor actor
actor SetMapper mapper

vtkRenderer ren
ren AddActor actor

vtkRenderWindow renWin
renWin AddRenderer ren

vtkRenderWindowInteractor iren
iren SetRenderWindow renWin

# This class allows all processes to composite their images.
# The root process then displays it in it's render window.
vtkTreeComposite tc
tc SetRenderWindow renWin
# Tell the pipeline that we requests pieces not the whole
# data.
tc InitializePieces
# Reset camera so that a global clipping range is computed
# before the first render
ren ResetCamera

iren Initialize
wm withdraw .

# Only the root process will have an active interactor. All
# the other render windows will be slaved to the root.
iren Start

# This has to be called to clean-up. If it is not called, MPI
# will most probably complain at exit.
controller Finalize
exit