#include <nsi.hpp>
#include <array>

int main(int argc, char **argv)
{
    NSI::Context nsi;

    NSI::ArgumentList args;
	args.Add(new NSI::StringArg("type","apistream"));
	args.Add(new NSI::StringArg("streamfilename","stdout"));
	nsi.Begin(args);
	{
		// Anchor
		nsi.Create("anchor_trs", "transform");
		std::array<double, 16> m = { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, -14, 1 } };
		nsi.SetAttribute("anchor_trs",
				NSI::DoubleMatrixArg("transformationmatrix", m.data()));
        nsi.Connect("anchor_trs","",".root","objects");
	}
	{
		// Procedural 2D gear
        NSI::ArgumentList args;
		args.Add(new NSI::StringArg("type","dynamiclibrary"));
		args.Add(new NSI::StringArg("filename","./hda_procedural"));
		args.Add(new NSI::StringArg("node","hda"));
		args.Add(new NSI::StringArg("parentnode","anchor_trs"));
		args.Add(new NSI::IntegerArg("nb_teeth",16));
		args.Add(new NSI::FloatArg("inner_radius",2.5));
		args.Add(new NSI::FloatArg("outer_radius",3.0));
		nsi.Evaluate(args);
		std::array<float,3> c = {0.2,0.3,0.4};
		nsi.SetAttribute("hda",
				NSI::ColorArg("Cs",c.data()));
	}
	{
		// Output
		nsi.Create("maincamera", "perspectivecamera");
		nsi.SetAttribute("maincamera",
				NSI::FloatArg("fov",30.0));
        nsi.Connect("maincamera","",".root","objects");
        //
		nsi.Create("raster", "screen");
		nsi.SetAttribute("raster",
				NSI::IntegerArg("oversampling",8));
        nsi.Connect("raster","","maincamera","screens");
        {
            NSI::ArgumentList args;
    		nsi.Create("cs", "outputlayer");
			args.Add(new NSI::StringArg("variablename","Cs"));
			args.Add(new NSI::StringArg("variablesource","attribute"));
			args.Add(new NSI::IntegerArg("withalpha",1));
			args.Add(new NSI::StringArg("scalarformat","half"));
			nsi.SetAttribute("cs",args);
            nsi.Connect("cs","","raster","outputlayers");
        }
        {
            NSI::ArgumentList args;
    		nsi.Create("out", "outputdriver");
			args.Add(new NSI::StringArg("drivername","exr"));
			args.Add(new NSI::StringArg("imagefilename","proc_eval_gear.exr"));
			nsi.SetAttribute("out",args);
	        nsi.Connect("out","","cs","outputdrivers");

        }
	}
	{
		// Render
        NSI::ArgumentList args;
        args.Add(new NSI::CStringPArg("action", "start"));
        nsi.RenderControl(args);
	}
    nsi.End();

	return 0;
}
