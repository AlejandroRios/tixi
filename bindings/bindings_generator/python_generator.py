# -*- coding: utf-8 -*-
"""
Created on Wed May 01 11:43:11 2013

@author: Martin Siggel <martin.siggel@dlr.de>
"""

class PythonGenerator(object):
    '''
    Generates Python code wrappers from the parsed semantics
    '''    
    
    def __init__(self, name_prefix, libname):
        self.prefix = name_prefix
        self.libname = libname
        self.classname = libname[0].upper() + libname[1:].lower()
        self.handle_str = '_handle'
        self.native_types = ['int', 'double', 'float', 'char', 'bool']
        self.license = None
        self.userfunctions = None
        self.closefunction = None
        self.aliases = {}
            
    def add_alias(self, oldname, newname):
        self.aliases[oldname] = newname
            
    def create_wrapper(self, cparser):
        string = '# -*- coding: utf-8 -*-\n'
        if self.license:
            string += self.license
        
        string += 'import sys, ctypes\n\n'
        for enumname, values in cparser.enums.iteritems():
            string += self.create_enum(enumname, values) + '\n\n'

        string += self.create_error_handler(cparser)+'\n\n'
        
        string += 'class %s(object):\n\n' % (self.classname)
        string += self.create_constructor() + '\n\n'
        string += self.create_destructor()  + '\n\n'
            
        if self.userfunctions:
            for line in self.userfunctions.splitlines():
                string += '    ' + line + '\n'
        
        for dec in cparser.declarations:
            string += self.create_method_wrapper(dec) + '\n\n'
            
        return string
        
    def create_error_handler(self, cparser):
        string = ''
        indent = 4*' '
        string += 'class %sException(Exception):\n' % self.classname
        string += '    \'\'\' The exception encapsulates the error return code of the library and arguments that were provided for the function. \'\'\'\n'
        string += '    def __init__(self, code, *args, **kwargs):\n'
        string += '        Exception.__init__(self)\n'
        string += '        self.code = code\n'
        string += '        if "error" in kwargs:\n'
        string += '            self.error = str(kwargs["error"])\n'
        string += '        elif code in %s._names:\n' % cparser.returncode_str
        string += '            self.error = %s._names[code]\n' % cparser.returncode_str
        string += '        else:\n'
        string += '            self.error = "UNDEFINED"\n'
        string += '        self.args = tuple(args)\n'
        string += '        self.kwargs = dict(kwargs)\n'
        string += '    def __str__(self):\n'
        string += '        return self.error + " (" + str(self.code) + ") " + str(list(self.args)) + " " + str(self.kwargs)\n'      
        string += '\n\n'
        
        if cparser.returncode_str in cparser.enums:
            successcode = '%s.%s' % (cparser.returncode_str, cparser.enums[cparser.returncode_str][0])
        else:
            successcode = '0'
        string += 'def catch_error(returncode, *args, **kwargs):\n'
        string += indent + 'if returncode != %s:\n' % successcode
        string += indent + '    raise %sException(returncode, args, kwargs)\n' \
            % self.classname        
        return string
                

    def create_constructor(self):
        indent = 4*' '
        string = ''
        string += indent + 'def __init__(self):\n'
        indent +=  4*' '
        string += indent + 'self.%s = ctypes.c_int(-1)\n\n' % self.handle_str
               
        string += indent + '# We only support python2.5 - 3.0 now\n'
        string += indent + 'if sys.version_info>(3,0,0):\n'
        string += indent + '    print("Python3 not supported in %sWrapper.")\n'\
            % self.prefix
        string += indent + '    sys.exit()\n'
        string += indent + 'elif sys.version_info<(2,5,0):\n'
        string += indent + '    print("At least python 2.5 is needed from %sWrapper.")\n\n' \
            % self.prefix
        
        string += indent + 'if sys.platform == \'win32\':\n'
        string += indent + '    self.lib = ctypes.cdll.%s\n' % self.libname
        string += indent + 'elif sys.platform == \'darwin\':\n'
        string += indent + '    self.lib = ctypes.CDLL("lib%s.dylib")\n' % self.libname
        string += indent + 'else:\n'
        string += indent + '    self.lib = ctypes.CDLL("lib%s.so")\n' % self.libname
        
        return string
        
    def create_destructor(self):
        if not self.closefunction or self.closefunction == '':
            return ' '
        
        indent = 4*' '
        string = indent + 'def __del__(self):\n'
        indent += indent
        string += indent + 'if hasattr(self, "%s"):\n' % self.libname
        string += indent + '    if self.%s != None:\n' % self.libname
        string += indent + '        self.%s()\n' % self.closefunction
        string += indent + '        self.%s = None\n' % self.libname
        return string

    def create_enum(self, enumname, values):
        string = ''
        string = 'class %s(object):\n' % enumname
        indent = 4*' '
        for index, val in enumerate(values):
            string += indent + '%s = %d\n' % (val, index)
        string += indent + '_names = {}\n'
        for index, val in enumerate(values):
            string += indent + '_names[%d] = \'%s\'\n' % (index, val)
        
        return string
    
    def create_header(self, fun_dec, indention_depth):
        '''Creates the method header as "def myFunc(self, myvalue):"'''
        
        string = ''
        indent = 4*indention_depth*' '
        
        # generate simplified method name, e.g.: tixiGetValue -> getValue
        name = fun_dec.method_name
        if name in self.aliases:
            name = self.aliases[name]
        else:
            if name.startswith(self.prefix):
                name = name[len(self.prefix)].lower() + name[len(self.prefix)+1:]
        
        # position of the handle variable
        handle_index = -1
        
        num_inargs  = 0
        num_outargs = 0
        
        # create method header
        string += indent + 'def %s(self' % name
        for index, arg in enumerate(fun_dec.arguments):
            if arg.is_handle and fun_dec.uses_handle:
                # we dont use the handle as an function argument
                handle_index = index
            elif not arg.is_outarg:
                string += ', %s' % arg.name
                num_inargs += 1
            elif arg.is_outarg:
                num_outargs += 1
            
        string += '):\n'
        return (string, num_inargs, num_outargs, handle_index)
    
    def create_pre_call(self, fun_dec, num_inargs, num_outargs, indention_depth):
        '''
        Creates the code for the input argument conversion and prepares the
        output arguments.
        '''
        indent = 4*(indention_depth+1)*' '
        string = ''
        raw_name = fun_dec.method_name
        
        #create input arguments
        if num_inargs > 0:
            string += indent + '# input arg conversion\n'
            
        iargs = (arg for arg in fun_dec.arguments if not arg.is_outarg)
        for arg in iargs:
            tmp_str = ''
            if arg.is_handle:
                continue
            elif arg.is_string:
                tmp_str = '_c_%s = ctypes.c_char_p(%s)' \
                    % (arg.name, arg.name)
            elif not arg.is_array and arg.npointer == 0:
                tmp_str = '_c_%s = ctypes.c_%s(%s)' \
                    % (arg.name, arg.type, arg.name)
            elif arg.is_array and arg.npointer > 0:
                # create type
                tmp_str = 'array_t_%s = ctypes.c_%s * len(%s)\n' \
                    % (arg.name, arg.type, arg.name)
                tmp_str += indent
                tmp_str += '_c_%s = array_t_%s(*%s)' \
                    % (arg.name, arg.name, arg.name)
            else:
                raise Exception('Cannot create python to c conversion ' +
                 'for input argument "%s" in "%s"' % (arg.name, raw_name))
            
            if not arg.type in self.native_types:
                raise Exception('Cannot create python to c conversion ' +
                 'for type "%s"' % fun_dec.method_name)
            
            string += indent + tmp_str + '\n'

        #create output arguments
        if num_outargs > 0:
            string += '\n'
            string += indent + '# output arg preparation\n'
        oargs = (arg for arg in fun_dec.arguments if arg.is_outarg)
        for arg in oargs:
            if arg.is_handle:
                continue
            elif arg.is_array and arg.npointer > 0:
                tmp_str = '_c_%s = ctypes.POINTER(ctypes.c_%s)()' \
                    % (arg.name, arg.type)
            elif arg.is_string:
                tmp_str = '_c_%s = ctypes.c_char_p()' % (arg.name)
            elif not arg.is_array and arg.npointer == 1:
                tmp_str = '_c_%s = ctypes.c_%s()' % (arg.name, arg.type)
            else:
                raise Exception('Cannot create python to c conversion ' +
                 'for output argument "%s" in %s' % (arg.name, raw_name) )
                
            string += indent + tmp_str + '\n'
                
        return string
        
    def create_call(self, fun_dec, ret_val, handle_index, indention_depth):
        '''
        Create function call code
        '''
        indent = 4*(indention_depth+1)*' '
        
        string = indent + '# call to native function\n'
        if ret_val:
            if ret_val.is_string:
                string +=  indent + 'self.lib.%s.restype = ctypes.c_char_p\n' \
                      % (fun_dec.method_name)
            else:
                string += indent + 'self.lib.%s.restype = ctypes.c_%s\n' \
                      % (fun_dec.method_name, ret_val.type)
            
        call = 'self.lib.%s(' % fun_dec.method_name
        for index, arg in enumerate(fun_dec.arguments):
            if index == handle_index and not arg.is_outarg:
                call += 'self.%s' % self.handle_str
            elif index == handle_index and arg.is_outarg:
                call += 'ctypes.byref(self.%s)' % self.handle_str
            elif arg.is_outarg:
                call += 'ctypes.byref(_c_%s)' % arg.name
            else:
                call += '_c_%s' % arg.name
                
            if index < len(fun_dec.arguments) - 1:
                call += ', '
                
        call += ')'
        
        # we assume that each function is returning an error code
        # only if explictly specified otherwise
        if not ret_val:
            call  = indent + 'errorCode = %s\n' % call
            call += indent + 'catch_error(errorCode'
            for index, arg in enumerate(fun_dec.arguments):
                if arg.is_handle and fun_dec.uses_handle:
                    # we dont use the handle as an function argument
                    pass
                elif not arg.is_outarg:
                    call += ', %s' % arg.name
                    
            call += ')'          
        else:
            call = indent + '_c_%s = %s' % (ret_val.name, call)
                
        
        string +=  call + '\n'
        return string
    
    def create_method_wrapper(self, fun_dec):
        '''
        Generates the python wrapper code around a c function call
        '''
        
        indent = 4*' '
        
        string, num_inargs, num_outargs, handle_index = \
            self.create_header(fun_dec, 1)
        
        ret_val = None if fun_dec.returns_error else fun_dec.return_value

        string += self.create_pre_call(fun_dec, num_inargs, num_outargs, 1)
        string += '\n'
                            
        string += self.create_call(fun_dec, ret_val, handle_index, 1)

        
        # accumulate return values
        outargs = []
        if ret_val:
            outargs.insert(0, ret_val)
        
        for arg in fun_dec.arguments:
            if arg.is_outarg:
                outargs.append(arg)
        
        # convert c ouputs to python output values
        if len(outargs) > 0:
            string += '\n'
        # non array value
        for arg in outargs:
            if not arg.is_array and not arg is ret_val and not arg.is_handle:
                tmp_str = '_py_%s = _c_%s.value' \
                    % (arg.name, arg.name)
            
                string += 2*indent + tmp_str + '\n'
            elif arg is ret_val:
                tmp_str = '_py_%s = _c_%s' \
                    % (arg.name, arg.name)
            
                string += 2*indent + tmp_str + '\n' 
        # arrays  
        for arg in outargs:
            if arg.is_array:
                # calculate size of array
                size_str = '%s_size =' % arg.name
                for sizeindex in arg.arraysizes:
                    sizearg = fun_dec.arguments[sizeindex]
                    if not sizearg.is_outarg:
                        size_str += ' %s *' % sizearg.name
                    else:
                        size_str += ' _py_%s *' % sizearg.name
                string += 2*indent + size_str[0:-1] + '\n'    
                
                
                tmp_str = '_py_%s = [_c_%s[i] for i in xrange(%s_size)]' \
                    % (arg.name, arg.name, arg.name)
            
                string += 2*indent + tmp_str + '\n'
                
        # remove size arguments from the return statement
        for index, arg in enumerate(outargs):
            if arg.is_sizearg or arg.is_handle:
                # we dont return a size arg, we just need it to create the array
                outargs.remove(arg)

        # create the return statement
        if len(outargs) == 1:
            string += '\n' + 2*indent + 'return _py_%s\n' % outargs[0].name
        elif len(outargs) > 1:
            string += '\n' + 2*indent + 'return ('
            for i, arg in enumerate(outargs):
                string += '_py_' + arg.name
                if i < len(outargs)-1:
                    string += ', '
            string += ')\n'
        
        return string + '\n'