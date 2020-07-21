#
# Example python script to generate a BOM from a KiCad generic netlist
#
# Example: Sorted and Grouped CSV BOM
#

"""
    @package
    Generate a CSV BOM for use with Digikey service.
    Components are sorted by ref and grouped by value with same footprint
    Fields are (if exist).
    DIGIKEY Part numbers are copied from the "DIGIKEY Part" field, if exists.
    It is possible to hide components from the BOM by setting the 
    "DIGIKEYPCB BOM" field to "0" or "false".

    Output fields:
    'Comment', 'Designator', 'Footprint', 'DIGIKEY Part #'

    Command line:
    python "pathToFile/bom_csv_digikey.py" "%I" "%O.csv"
"""

import csv
import sys
import os

sys.path.append("/usr/share/kicad/plugins")

import kicad_netlist_reader


net = kicad_netlist_reader.netlist(sys.argv[1])

#with open(sys.argv[2], 'w', newline='') as f:
with open(sys.argv[2], 'w') as f:
    out = csv.writer(f)
    out.writerow(['Comment', 'Designator', 'Footprint', 'DIGIKEY Part #'])

    for group in net.groupComponents():
        refs = []

        digikey_pn = ""
        for component in group:
            if component.getField('DIGIKEYPCB BOM') in ['0', 'false', 'False', 'FALSE']:
                continue
            refs.append(component.getRef())
            digikey_pn = component.getField("digikey") or digikey_pn
            c = component

        if len(refs) == 0:
            continue

        # Fill in the component groups common data
        out.writerow([c.getValue() + " " + c.getDescription(), ",".join(refs), c.getFootprint().split(':')[1],
            digikey_pn])

    f.close()
