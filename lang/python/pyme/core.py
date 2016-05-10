# $Id$
# Copyright (C) 2004,2008 Igor Belyi <belyi@users.sourceforge.net>
# Copyright (C) 2002 John Goerzen <jgoerzen@complete.org>
#
#    This library is free software; you can redistribute it and/or
#    modify it under the terms of the GNU Lesser General Public
#    License as published by the Free Software Foundation; either
#    version 2.1 of the License, or (at your option) any later version.
#
#    This library is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    Lesser General Public License for more details.
#
#    You should have received a copy of the GNU Lesser General Public
#    License along with this library; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA

# import generators for portability with python2.2


from . import pygpgme
from .errors import errorcheck
from . import errors
from .util import GpgmeWrapper

class Context(GpgmeWrapper):
    """From the GPGME C documentation:

    * All cryptographic operations in GPGME are performed within a
    * context, which contains the internal state of the operation as well as
    * configuration parameters.  By using several contexts you can run
    * several cryptographic operations in parallel, with different
    * configuration.

    Thus, this is the place that you will usually start."""

    def _getctype(self):
        return 'gpgme_ctx_t'
    
    def _getnameprepend(self):
        return 'gpgme_'

    def _errorcheck(self, name):
        """This function should list all functions returning gpgme_error_t"""
        if (name.startswith('gpgme_op_') and \
            not name.endswith('_result')) or \
            name == 'gpgme_signers_add' or \
            name == 'gpgme_set_locale' or \
            name == 'gpgme_set_keylist_mode' or \
            name == 'gpgme_set_protocol':
            return 1
        return 0

    def __init__(self, wrapped=None):
        if wrapped:
            self.wrapped = wrapped
            self.own = False
        else:
            tmp = pygpgme.new_gpgme_ctx_t_p()
            errorcheck(pygpgme.gpgme_new(tmp))
            self.wrapped = pygpgme.gpgme_ctx_t_p_value(tmp)
            pygpgme.delete_gpgme_ctx_t_p(tmp)
            self.own = True
        self.last_passcb = None
        self.last_progresscb = None

    def __del__(self):
        self._free_passcb()
        self._free_progresscb()
        if self.own:
            pygpgme.gpgme_release(self.wrapped)

    def _free_passcb(self):
        if self.last_passcb != None:
            pygpgme.pygpgme_clear_generic_cb(self.last_passcb)
            pygpgme.delete_PyObject_p_p(self.last_passcb)
            self.last_passcb = None

    def _free_progresscb(self):
        if self.last_progresscb != None:
            pygpgme.pygpgme_clear_generic_cb(self.last_progresscb)
            pygpgme.delete_PyObject_p_p(self.last_progresscb)
            self.last_progresscb = None

    def op_keylist_all(self, *args, **kwargs):
        self.op_keylist_start(*args, **kwargs)
        key = self.op_keylist_next()
        while key:
            yield key
            key = self.op_keylist_next()

    def op_keylist_next(self):
        """Returns the next key in the list created
        by a call to op_keylist_start().  The object returned
        is of type Key."""
        ptr = pygpgme.new_gpgme_key_t_p()
        try:
            errorcheck(pygpgme.gpgme_op_keylist_next(self.wrapped, ptr))
            key = pygpgme.gpgme_key_t_p_value(ptr)
        except errors.GPGMEError as excp:
            key = None
            if excp.getcode() != errors.EOF:
                raise excp
        pygpgme.delete_gpgme_key_t_p(ptr)
        if key:
            key.__del__ = lambda self: pygpgme.gpgme_key_unref(self)
            return key
    
    def get_key(self, fpr, secret):
        """Return the key corresponding to the fingerprint 'fpr'"""
        ptr = pygpgme.new_gpgme_key_t_p()
        errorcheck(pygpgme.gpgme_get_key(self.wrapped, fpr, ptr, secret))
        key = pygpgme.gpgme_key_t_p_value(ptr)
        pygpgme.delete_gpgme_key_t_p(ptr)
        if key:
            key.__del__ = lambda self: pygpgme.gpgme_key_unref(self)
            return key

    def op_trustlist_all(self, *args, **kwargs):
        self.op_trustlist_start(*args, **kwargs)
        trust = self.ctx.op_trustlist_next()
        while trust:
            yield trust
            trust = self.ctx.op_trustlist_next()

    def op_trustlist_next(self):
        """Returns the next trust item in the list created
        by a call to op_trustlist_start().  The object returned
        is of type TrustItem."""
        ptr = pygpgme.new_gpgme_trust_item_t_p()
        try:
            errorcheck(pygpgme.gpgme_op_trustlist_next(self.wrapped, ptr))
            trust = pygpgme.gpgme_trust_item_t_p_value(ptr)
        except errors.GPGMEError as excp:
            trust = None
            if excp.getcode() != errors.EOF:
                raise
        pygpgme.delete_gpgme_trust_item_t_p(ptr)
        return trust

    def set_passphrase_cb(self, func, hook=None):
        """Sets the passphrase callback to the function specified by func.

        When the system needs a passphrase, it will call func with three args:
        hint, a string describing the key it needs the passphrase for;
        desc, a string describing the passphrase it needs;
        prev_bad, a boolean equal True if this is a call made after
        unsuccessful previous attempt.
        
        If hook has a value other than None it will be passed into the func
        as a forth argument.

        Please see the GPGME manual for more information.
        """
        self._free_passcb()
        if func == None:
            hookdata = None
        else:
            self.last_passcb = pygpgme.new_PyObject_p_p()
            if hook == None:
                hookdata = func
            else:
                hookdata = (func, hook)
        pygpgme.pygpgme_set_passphrase_cb(self.wrapped, hookdata, self.last_passcb)

    def set_progress_cb(self, func, hook=None):
        """Sets the progress meter callback to the function specified by

        This function will be called to provide an interactive update of
        the system's progress.

        Please see the GPGME manual for more information."""
        self._free_progresscb()
        if func == None:
            hookdata = None
        else:
            self.last_progresscb = pygpgme.new_PyObject_p_p()
            if hook == None:
                hookdata = func
            else:
                hookdata = (func, hook)
        pygpgme.pygpgme_set_progress_cb(self.wrapped, hookdata, self.last_progresscb)

    def get_engine_info(self):
        """Returns this context specific engine info"""
        return pygpgme.gpgme_ctx_get_engine_info(self.wrapped)

    def set_engine_info(self, proto, file_name, home_dir=None):
        """Changes the configuration of the crypto engine implementing the
    protocol 'proto' for the context. 'file_name' is the file name of
    the executable program implementing this protocol. 'home_dir' is the
    directory name of the configuration directory (engine's default is
    used if omitted)."""
        errorcheck(pygpgme.gpgme_ctx_set_engine_info(self.wrapped, proto, file_name, home_dir))

    def wait(self, hang):
        """Wait for asynchronous call to finish. Wait forever if hang is True

        Return:
            On an async call completion its return status.
            On timeout - None.

        Please read the GPGME manual for more information."""
        ptr = pygpgme.new_gpgme_error_t_p()
        context = pygpgme.gpgme_wait(self.wrapped, ptr, hang)
        status = pygpgme.gpgme_error_t_p_value(ptr)
        pygpgme.delete_gpgme_error_t_p(ptr)
        
        if context == None:
            errorcheck(status)
            return None
        else:
            return status

    def op_edit(self, key, func, fnc_value, out):
        """Start key editing using supplied callback function"""
        if key == None:
            raise ValueError("op_edit: First argument cannot be None")
        opaquedata = (func, fnc_value)
        errorcheck(pygpgme.gpgme_op_edit(self.wrapped, key, opaquedata, out))
    
class Data(GpgmeWrapper):
    """From the GPGME C manual:

* A lot of data has to be exchanged between the user and the crypto
* engine, like plaintext messages, ciphertext, signatures and information
* about the keys.  The technical details about exchanging the data
* information are completely abstracted by GPGME.  The user provides and
* receives the data via `gpgme_data_t' objects, regardless of the
* communication protocol between GPGME and the crypto engine in use.

        This Data class is the implementation of the GpgmeData objects.

        Please see the information about __init__ for instantiation."""

    def _getctype(self):
        return 'gpgme_data_t'
    
    def _getnameprepend(self):
        return 'gpgme_data_'

    def _errorcheck(self, name):
        """This function should list all functions returning gpgme_error_t"""
        if name == 'gpgme_data_release_and_get_mem' or \
               name == 'gpgme_data_get_encoding' or \
               name == 'gpgme_data_seek':
            return 0
        return 1
    
    def __init__(self, string = None, file = None, offset = None,
                 length = None, cbs = None):
        """Initialize a new gpgme_data_t object.

        If no args are specified, make it an empty object.

        If string alone is specified, initialize it with the data
        contained there.

        If file, offset, and length are all specified, file must
        be either a filename or a file-like object, and the object
        will be initialized by reading the specified chunk from the file.

        If cbs is specified, it MUST be a tuple of the form:

        ((read_cb, write_cb, seek_cb, release_cb), hook)

        where func is a callback function taking two arguments (count,
        hook) and returning a string of read data, or None on EOF.
        This will supply the read() method for the system.

        If file is specified without any other arguments, then
        it must be a filename, and the object will be initialized from
        that file.

        Any other use will result in undefined or erroneous behavior."""
        self.wrapped = None
        self.last_readcb = None

        if cbs != None:
            self.new_from_cbs(*cbs)
        elif string != None:
            self.new_from_mem(string)
        elif file != None and offset != None and length != None:
            self.new_from_filepart(file, offset, length)
        elif file != None:
            if type(file) == type("x"):
                self.new_from_file(file)
            else:
                self.new_from_fd(file)
        else:
            self.new()

    def __del__(self):
        if self.wrapped != None:
            pygpgme.gpgme_data_release(self.wrapped)
        self._free_readcb()

    def _free_readcb(self):
        if self.last_readcb != None:
            pygpgme.pygpgme_clear_generic_cb(self.last_readcb)
            pygpgme.delete_PyObject_p_p(self.last_readcb)
            self.last_readcb = None

    def new(self):
        tmp = pygpgme.new_gpgme_data_t_p()
        errorcheck(pygpgme.gpgme_data_new(tmp))
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_mem(self, string, copy = 1):
        tmp = pygpgme.new_gpgme_data_t_p()
        errorcheck(pygpgme.gpgme_data_new_from_mem(tmp,string,len(string),copy))
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_file(self, filename, copy = 1):
        tmp = pygpgme.new_gpgme_data_t_p()
        errorcheck(pygpgme.gpgme_data_new_from_file(tmp, filename, copy))
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_cbs(self, funcs, hook):
        """Argument funcs must be a 4 element tuple with callbacks:
        (read_cb, write_cb, seek_cb, release_cb)"""
        tmp = pygpgme.new_gpgme_data_t_p()
        self._free_readcb()
        self.last_readcb = pygpgme.new_PyObject_p_p()
        hookdata = (funcs, hook)
        pygpgme.pygpgme_data_new_from_cbs(tmp, hookdata, self.last_readcb)
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_filepart(self, file, offset, length):
        """This wraps the GPGME gpgme_data_new_from_filepart() function.
        The argument "file" may be:

        1. a string specifying a file name, or
        3. a a file-like object. supporting the fileno() call and the mode
           attribute."""

        tmp = pygpgme.new_gpgme_data_t_p()
        filename = None
        fp = None

        if type(file) == type("x"):
            filename = file
        else:
            fp = pygpgme.fdopen(file.fileno(), file.mode)
            if fp == None:
                raise ValueError("Failed to open file from %s arg %s" % \
                      (str(type(file)), str(file)))

        errorcheck(pygpgme.gpgme_data_new_from_filepart(tmp, filename, fp,
                                                      offset, length))
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_fd(self, file):
        """This wraps the GPGME gpgme_data_new_from_fd() function.
        The argument "file" may be a file-like object, supporting the fileno()
        call and the mode attribute."""
        
        tmp = pygpgme.new_gpgme_data_t_p()
        fp = pygpgme.fdopen(file.fileno(), file.mode)
        if fp == None:
            raise ValueError("Failed to open file from %s arg %s" % \
                  (str(type(file)), str(file)))
        errorcheck(gpgme_data_new_from_fd(tmp, fp))
        self.wrapped = pygpgme.gpgme_data_t_p_value(tmp)
        pygpgme.delete_gpgme_data_t_p(tmp)

    def new_from_stream(self, file):
        """This wrap around gpgme_data_new_from_stream is an alias for
        new_from_fd() method since in python there's not difference
        between file stream and file descriptor"""
        self.new_from_fd(file)
    
    def write(self, buffer):
        errorcheck(pygpgme.gpgme_data_write(self.wrapped, buffer, len(buffer)))

    def read(self, size = -1):
        """Read at most size bytes, returned as a string.
        
        If the size argument is negative or omitted, read until EOF is reached.

        Returns the data read, or the empty string if there was no data
        to read before EOF was reached."""
        
        if size == 0:
            return ''

        if size > 0:
            return pygpgme.gpgme_data_read(self.wrapped, size)
        else:
            retval = ''
            while 1:
                result = pygpgme.gpgme_data_read(self.wrapped, 10240)
                if len(result) == 0:
                    break
                retval += result
            return retval

def pubkey_algo_name(algo):
    return pygpgme.gpgme_pubkey_algo_name(algo)

def hash_algo_name(algo):
    return pygpgme.gpgme_hash_algo_name(algo)

def get_protocol_name(proto):
    return pygpgme.gpgme_get_protocol_name(proto)

def check_version(version=None):
    return pygpgme.gpgme_check_version(version)

def engine_check_version (proto):
    try:
        errorcheck(pygpgme.gpgme_engine_check_version(proto))
        return True
    except errors.GPGMEError:
        return False

def get_engine_info():
    ptr = pygpgme.new_gpgme_engine_info_t_p()
    try:
        errorcheck(pygpgme.gpgme_get_engine_info(ptr))
        info = pygpgme.gpgme_engine_info_t_p_value(ptr)
    except errors.GPGMEError:
        info = None
    pygpgme.delete_gpgme_engine_info_t_p(ptr)
    return info

def set_engine_info(proto, file_name, home_dir=None):
    """Changes the default configuration of the crypto engine implementing
    the protocol 'proto'. 'file_name' is the file name of
    the executable program implementing this protocol. 'home_dir' is the
    directory name of the configuration directory (engine's default is
    used if omitted)."""
    errorcheck(pygpgme.gpgme_set_engine_info(proto, file_name, home_dir))

def set_locale(category, value):
    """Sets the default locale used by contexts"""
    errorcheck(pygpgme.gpgme_set_locale(None, category, value))

def wait(hang):
    """Wait for asynchronous call on any Context  to finish.
    Wait forever if hang is True.
    
    For finished anynch calls it returns a tuple (status, context):
        status  - status return by asnynchronous call.
        context - context which caused this call to return.
        
    Please read the GPGME manual of more information."""
    ptr = pygpgme.new_gpgme_error_t_p()
    context = pygpgme.gpgme_wait(None, ptr, hang)
    status = pygpgme.gpgme_error_t_p_value(ptr)
    pygpgme.delete_gpgme_error_t_p(ptr)
    if context == None:
        errorcheck(status)
    else:
        context = Context(context)
    return (status, context)

