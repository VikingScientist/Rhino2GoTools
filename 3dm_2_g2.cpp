#include "opennurbs.h"
#include "GoTools/geometry/SplineCurve.h"
#include "GoTools/geometry/ObjectHeader.h"
#include <iostream>
#include <fstream>

using namespace std;

int main( int argc, const char *argv[] )
{
	// If you are using OpenNURBS as a Windows DLL, then you MUST use
	// ON::OpenFile() to open the file.  If you are not using OpenNURBS
	// as a Windows DLL, then you may use either ON::OpenFile() or fopen()
	// to open the file.

	int argi;
	bool verbose; // verbose output
	if ( argc < 2 ) 
	{
		printf("Syntax: %s [-v] [-out:outputfilename.g2] file1.3dm file2.3dm ...\n",argv[0] );
		return 0;
	}

	// Call once in your application to initialze opennurbs library
	ON::Begin();

	// default dump is to stdout
	ON_TextLog dump_to_stdout;
	ON_TextLog* dump = &dump_to_stdout;
	FILE* dump_fp = 0;

	// g2 output
	ofstream *g2_to_file = NULL;
	ostream *g2_out = &cout;

	ONX_Model model;

	bool bVerboseTextDump = false;

	for ( argi = 1; argi < argc; argi++ ) 
	{
		const char* arg = argv[argi];

		// check for -out or /out option
		if ( ( 0 == strncmp(arg,"-out:",5) || 0 == strncmp(arg,"/out:",5) ) 
			   && arg[5] )
		{
			const char* fileName = arg+5;
			g2_to_file = new ofstream;
			g2_to_file->open(fileName);
			g2_out = (ostream*) g2_to_file;
			continue;
		} else if(strcmp(arg, "-v")==0 || strcmp(arg, "-V")==0 )
			verbose = true;

		const char* sFileName = arg;
		(*g2_out) << setprecision (17);

		if(verbose)
			dump->Print("\nOpenNURBS Archive File:  %s\n", sFileName );

		// open file containing opennurbs archive
		FILE* archive_fp = ON::OpenFile( sFileName, "rb");
		if ( !archive_fp ) 
		{
			dump->Print("  Unable to open file.\n" );
			continue;
		}

		if(verbose)
			dump->PushIndent();

		// create achive object from file pointer
		ON_BinaryFile archive( ON::read3dm, archive_fp );

		// read the contents of the file into "model"
		bool rc = model.Read( archive, dump );

		// close the file
		ON::CloseFile( archive_fp );

		// print diagnostic
		if ( rc ) {
			if(verbose) dump->Print("Successfully read.\n");
		} else {
			dump->Print("Errors during reading.\n");
		}

		// see if everything is in good shape
		if ( model.IsValid(dump)) {
			if(verbose) dump->Print("Model is valid.\n");
		} else {
			dump->Print("Model is not valid.\n");
		}

		// create a text dump of the model
		if ( bVerboseTextDump )
		{
			dump->PushIndent();
			model.Dump(*dump);
			dump->PopIndent();
		}

		// draw out the required geometry
		if(verbose)
			printf("Model consists of %d objects\n", model.m_object_table.Count());
		for(int i=0; i<model.m_object_table.Count(); i++) {
			ON_String obj_name = model.m_object_table[i].m_attributes.m_name;
			if(obj_name.Length() > 0) {
				const char * fileName = obj_name + ".g2";
				g2_to_file = new ofstream;
				g2_to_file->open(fileName, ios::out | ios::app);
				g2_out = (ostream*) g2_to_file;
			} else {
				g2_out = &cout;
			}
			(*g2_out) << setprecision(17);

			switch(model.m_object_table[i].m_object->ObjectType()) {
			case ON::curve_object: {
				if(verbose) {
					printf("object #%d is a curve object\n", i);
					printf("object name : \"%s\"\n", (const char*) obj_name);
				}
				ON_Curve      *curve   = (ON_Curve*) model.m_object_table[i].m_object;
				ON_NurbsCurve *n_curve = NULL;

				// curve = (ON_NurbsCurve*) model.m_object_table[i].m_object;
				n_curve = curve->NurbsCurve();
				if( !n_curve ) {
					fprintf(stderr, "Error converting curve to NURBS curve\nDumping curve details:\n");
					curve->Dump(*dump);
					exit(1);
				}
				ON_Interval domain = n_curve->Domain();
				if(n_curve->IsPeriodic()) {
					if(!n_curve->InsertKnot(domain.Min(), n_curve->Order()-1) || 
					   !n_curve->InsertKnot(domain.Max(), n_curve->Order()-1)  ) {
						fprintf(stderr, "Error converting curve to non-periodic\nDumping curve details:\n");
						n_curve->Dump(*dump);
						exit(1);
					}
				}
				double knot_vector[n_curve->KnotCount()+2]; // g2 is operating with knot multiplicity Order() at the start and end knot, openNURBS is not
				double *coefs = NULL; // g2 is storing controlpoints as P1_x*w,P1_y*w,P1_z*w,w P2_x*w,... that is each controlpoint is premultiplied by its weight. openNURBS is not
				if(n_curve->IsRational()) {
					coefs = new double[n_curve->CVCount() * n_curve->CVSize()]; // memleak I guess since I never delete this, but who cares. It's a small reading program which doesn't use too much memory anyway.
					int k=0;
					for(int j=0; j<n_curve->CVCount(); j++) {
						for(int d=0; d<n_curve->Dimension(); d++) {
							coefs[k++] = n_curve->CV(j)[d] * n_curve->Weight(j);
						}
						coefs[k++] = n_curve->Weight(j);
					}
				} else {
					coefs = n_curve->m_cv;
				}
				knot_vector[0] = n_curve->SuperfluousKnot(0);
				copy(n_curve->Knot(), n_curve->Knot() + n_curve->KnotCount(), knot_vector+1);
				knot_vector[n_curve->KnotCount()+1] = n_curve->SuperfluousKnot(1);

				Go::SplineCurve converted_curve(n_curve->CVCount(), n_curve->Order(), knot_vector, coefs, n_curve->Dimension(), n_curve->IsRational());
				Go::ObjectHeader head(Go::Class_SplineCurve, 1, 0);

				(*g2_out) << head;
				(*g2_out) << converted_curve;
				
				break;
			}
			case ON::brep_object: {
				if(verbose)
					printf("object #%d is a brep object ", i);
				// model.m_object_table[i].m_object->Dump(*dump);
				// printf("\n\n==========================================================================================\n\n");
				ON_Brep *brep_obj = (ON_Brep*) model.m_object_table[i].m_object;
				// brep_obj->Dump(*dump);
				int surfCount = 0;
				while(brep_obj->FaceIsSurface(surfCount)) {
					ON_Surface *surf = brep_obj->m_S[surfCount++];
					ON_NurbsSurface *n_surf = NULL;
					if(surf->HasNurbForm()) {
						if(verbose) printf("(NURBS surface)\n");
						 n_surf = surf->NurbsSurface();
						
					} else {
						if(verbose) printf("(non-NURBS surface) - ");
						n_surf = ON_NurbsSurface::New();
						if(surf->GetNurbForm(*n_surf) && verbose)
							printf("converted succesfully\n");
						else if(verbose)
							printf("not possible to convert\n");
					}
					if(n_surf) {
						// openNURBS is storing major index first, goTools minor index
						double x_comp[n_surf->m_cv_count[0]][n_surf->m_cv_count[1]];
						double y_comp[n_surf->m_cv_count[0]][n_surf->m_cv_count[1]];
						double z_comp[n_surf->m_cv_count[0]][n_surf->m_cv_count[1]];
						double w_comp[n_surf->m_cv_count[0]][n_surf->m_cv_count[1]];
						int k=0;
						for(int i=0; i<n_surf->m_cv_count[0]; i++) {
							for(int j=0; j<n_surf->m_cv_count[1]; j++) {
								x_comp[i][j] = n_surf->m_cv[k++];
								y_comp[i][j] = n_surf->m_cv[k++];
								z_comp[i][j] = n_surf->m_cv[k++];
								if(n_surf->m_is_rat)
									w_comp[i][j] = n_surf->m_cv[k++];
							}
						}

						Go::ObjectHeader head(Go::Class_SplineSurface, 1, 0);
						(*g2_out) << head;
						(*g2_out) << n_surf->m_dim         << " " << n_surf->m_is_rat   << endl;

						(*g2_out) << n_surf->m_cv_count[0] << " " << n_surf->m_order[0] << endl;
						(*g2_out) << n_surf->m_knot[0][0] << " "; // g2 operating w/ knot mult Order() at the start/end, openNURBS is not
						for(int i=0; i<n_surf->m_knot_capacity[0]; i++)
							(*g2_out) << n_surf->m_knot[0][i] << " ";
						(*g2_out) << n_surf->m_knot[0][n_surf->m_knot_capacity[0]-1] << endl;
						(*g2_out) << n_surf->m_cv_count[1] << " " << n_surf->m_order[1] << endl;
						(*g2_out) << n_surf->m_knot[1][0] << " "; // g2 operating w/ knot mult Order() at the start/end, openNURBS is not
						for(int i=0; i<n_surf->m_knot_capacity[1]; i++)
							(*g2_out) << n_surf->m_knot[1][i] << " ";
						(*g2_out) << n_surf->m_knot[1][n_surf->m_knot_capacity[1]-1] << endl;

						for(int j=0; j<n_surf->m_cv_count[1]; j++) {
							for(int i=0; i<n_surf->m_cv_count[0]; i++) {
								(*g2_out) << x_comp[i][j] << " " << y_comp[i][j] << " " << z_comp[i][j];
								if(n_surf->m_is_rat)
									(*g2_out) << " " << w_comp[i][j] ;
								(*g2_out) << endl;
							}
						}
					}
				}
				if( !brep_obj->IsSurface()) {
					if(verbose)
						printf("(non-surface) - and face 0 is not either a surface (maybe the others if there are?)\n");
				}
				break;
			}
			case ON::surface_object:
				if(verbose)
					printf("object #%d is a surface\n", i);
				break;
			case ON::mesh_object:
				if(verbose)
					printf("object #%d is a mesh object\n", i);
				break;
			case ON::layer_object:
				if(verbose)
					printf("object #%d is a layer object\n", i);
				break;
			case ON::point_object:
			case ON::pointset_object:
			case ON::material_object:
			case ON::light_object:
			case ON::annotation_object:
			case ON::userdata_object:
			case ON::text_dot:
				if(verbose)
					printf("object #%d is an other known type\n", i);
				break;
			case ON::unknown_object_type:
			default:
				if(verbose)
					printf("object #%d is an unknown type thingie\n", i);
				break;
			}
			if(g2_to_file)
				g2_to_file->close();
		}


		// destroy this model
		model.Destroy();

		dump->PopIndent();
	}

	if(g2_to_file)
		g2_to_file->close();

	if ( dump_fp )
	{
		// close the text dump file
		delete dump;
		ON::CloseFile( dump_fp );
	}
	
	// OPTIONAL: Call just before your application exits to clean
	//           up opennurbs class definition information.
	//           Opennurbs will not work correctly after ON::End()
	//           is called.
	ON::End();

	return 0;
}

