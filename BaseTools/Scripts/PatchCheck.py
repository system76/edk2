                not lines[i].startswith('git-svn-id:')):
                if self.filename.endswith('.sh'):
                if self.filename == '.gitmodules':
                    # .gitmodules is updated by git and uses tabs and LF line
                    # endings.  Do not enforce no tabs and do not enforce
                    # CR/LF line endings.
        if self.force_crlf and eol != '\r\n':