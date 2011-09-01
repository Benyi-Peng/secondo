#!/bin/sh

################################################################################
### Description:
### - This shell-script imports OSM data from the file
### http://download.geofabrik.de/osm/europe/germany/nordrhein-westfalen/
### <name>.shp.zip, from self-made shape-files, e.g. produced with tools like
### osm2shp from a xml-based osm-file, or directly from a osm-file.
### In case of zipped shape-files download the file, unzip and extract it.
### Afterwards make a few manual modifications in the shell-script at hand.
### First adapt the city name and the clipping rectangle, then set SRC_DIR_PATH
### to your local extraction directory.
### This shell script changes the settings in a companion SECONDO-script
### and executes another script which controls the import process.
###
### Author:
### - Thomas Uchdorf, t.uchdorf@arcor.de
################################################################################

# --- feel free to perform manual changes in the following part
# Determining city-specific settings
# Münster
#name='muenster'
#min_lon=7.52
#max_lon=7.74
#min_lat=51.93
#max_lat=51.99

# Dortmund
#name='dortmund'
#min_lon=7.303333
#max_lon=7.638889
#min_lat=51.416944
#max_lat=51.601389

# Düsseldorf
#name='hometown'
#name='duesseldorf'
name='beispiel'
min_lon=6.65
max_lon=6.91
min_lat=51.18
max_lat=51.28
#min_lon=6.70
#max_lon=6.80
#min_lat=51.20
#max_lat=51.25

# Specifying the path to the directory in which the data is stored
#src_dir_path="/Users/fernuni-student/secondo-data/shp-files/${name}/"
src_dir_path="/Users/fernuni-student/secondo-data/osm-files/${name}/"

# Limiting the region of interest if desired
# north-west (NW), west (W), south-west (SW), north (N), centre (C), south (S),
# north-east (NE), east (E), south-east (SE), or another random string
part='C'
#part='City'

# Defining the format of the shape-file that is to be processed
#file_type='shp_geofabrik'
#file_type='shp_osm2shp'
file_type='osm'

# --- please only modify the subsequent lines if you exactly know what you are
#     doing
# Modifying a SECONDO-file that serves as template
#capitalizedName=`echo ${name} | cut -c1 | tr '[a-z]' '[A-Z]'``echo ${name} | cut -c2-`
inp='TuPreprocessImport.sec.tmpl'
outp="TuPreprocessImport.sec"
echo "Creating file \"${outp}\"..."
expr1="s#<name>#${name}#g"
expr2="s#<src_dir_path>#${src_dir_path}#g"
expr3="s#<min_lon>#${min_lon}#g"
expr4="s#<max_lon>#${max_lon}#g"
expr5="s#<min_lat>#${min_lat}#g"
expr6="s#<max_lat>#${max_lat}#g"
expr7="s#<part>#${part}#g"
sed -E ${expr1} ${inp}| sed -E  ${expr2} | sed -E  ${expr3}| sed -E  ${expr4}|
sed -E  ${expr5}| sed -E  ${expr6} | sed -E  ${expr7} > ${outp}

# Starting the import
if test ${file_type} = 'osm'; then
   SecondoTTYBDB -i TuOsmImport.sec
elif test ${file_type} = 'shp_geofabrik'; then
   SecondoTTYBDB -i TuShpImport.sec
else
   SecondoTTYBDB -i TuCustomShpImport.sec
fi
