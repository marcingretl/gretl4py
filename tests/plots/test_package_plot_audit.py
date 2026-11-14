"""Unit tests for the offline part of the package plot audit."""
from __future__ import annotations
import io
from pathlib import Path
import tempfile
import unittest
from zipfile import ZipFile
from package_plot_audit import catalog_from_html, extract_gfn_sections, source_evidence, iter_files

GFN = b'''<?xml version="1.0" encoding="UTF-8"?>
<gretl-function-package name="Example">
  <gretl-function name="draw" type="void">
    <code>matrix M = zeros(5, 2)\ngnuplot 1 2 --with-lines --output=display</code>
  </gretl-function>
  <gretl-function name="compute" type="scalar">
    <code>return 3</code>
  </gretl-function>
  <sample-script>include Example.gfn\nopen denmark\ndraw()</sample-script>
</gretl-function-package>'''

class ScannerTests(unittest.TestCase):
    def test_catalog_html(self):
        html = '<a href="/current_fnfiles/A.gfn">A</a><a href="/current_fnfiles/B.zip">B</a><a href="help.pdf">docs</a>'
        links = catalog_from_html(html)
        self.assertEqual(set(links), {'A.gfn', 'B.zip'})

    def test_sections(self):
        found = extract_gfn_sections(GFN)
        self.assertEqual(len(found), 3)
        self.assertEqual(found[0][1], 'draw')
        self.assertEqual(found[-1][0], 'sample')

    def test_detect_function_plot(self):
        evidence, sections, errors = source_evidence('Example', 'local', 'Example.gfn', GFN)
        self.assertEqual(sections, 3)
        self.assertEqual(errors, 0)
        self.assertTrue(any(x.kind == 'direct_plot' and x.function == 'draw' for x in evidence))

    def test_scan_zip_without_extracting(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            with ZipFile(root / 'Example.zip', 'w') as z:
                z.writestr('Example/Example.gfn', GFN)
                z.writestr('../ESCAPE.gfn', GFN)
            files = list(iter_files(root))
            self.assertEqual(len(files), 1)
            self.assertIn('Example.gfn', files[0][2])

if __name__ == '__main__':
    unittest.main()
