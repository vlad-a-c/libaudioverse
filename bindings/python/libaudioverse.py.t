{%-import 'macros.t' as macros with context-%}
import _lav
import _libaudioverse
import weakref
import collections
import ctypes

{%macro implement_property(enumerant, prop)%}
	@property
	def {{prop['name']}}(self):
		return _lav.object_get_{{prop['type']}}_property(self.handle, _libaudioverse.{{enumerant}})

	@{{prop['name']}}.setter
	def {{prop['name']}}(self, val):
{%if prop['type'] == 'int'%}
		_lav.object_set_int_property(self.handle, _libaudioverse.{{enumerant}}, int(val))
{%elif prop['type'] == 'float' or prop['type'] == 'double'%}
		_lav.object_set_{{prop['type']}}_property(self.handle, _libaudioverse.{{enumerant}}, float(val))
{%elif prop['type'] == 'float3'%}
		arg_tuple = tuple(val)
		if len(arg_tuple) != 3:
			raise  ValueError('Expected a list or list-like object of 3 floats')
		_lav.object_set_float3_property(self.handle, _libaudioverse.{{enumerant}}, *(float(i) for i in arg_tuple))
{%elif prop['type'] == 'float6'%}
		arg_tuple = tuple(val)
		if len(arg_tuple) != 6:
			raise ValueError('Expected a list or list-like object of 6 floats')
		_lav.object_set_float6_property(self.handle, _libaudioverse.{{enumerant}}, *(float(i) for i in arg_tuple))
{%else%}
		pass
{%endif%}
{%endmacro%}

{%macro implement_callback(name, index)%}
	@property
	def {{name}}_callback(self):
		cb = self._callbacks.get({{index}}, None)
		if cb is None:
			return
		return (cb.callback, cb.extra_arguments)

	@{{name}}_callback.setter
	def {{name}}_callback(self, val):
		global _global_callbacks
		val_tuple = tuple(val) if isinstance(val, collections.Iterable) else (val, )
		if len(val_tuple) == 1:
			val_tuple = (val, ())
		cb, extra_args = val_tuple
		callback_obj = _EventCallbackWrapper(self, {{index}}, cb, extra_args)
		self._callbacks[{{index}}] = callback_obj
		_global_callbacks[self.handle].add(callback_obj)
{%endmacro%}

#initialize libaudioverse.  This is per-app and implies no context settings, etc.
_lav.initialize_library()

#this makes sure that callback objects do not die.
_global_callbacks = collections.defaultdict(set)

#build and register all the error classes.
class GenericError(Exception):
	"""Base for all libaudioverse errors."""
	pass

{%for error_name in constants.iterkeys()|prefix_filter("Lav_ERROR_")|remove_filter("Lav_ERROR_NONE")%}
{%set friendly_name = error_name|strip_prefix("Lav_ERROR_")|lower|underscores_to_camelcase(True)%}
class {{friendly_name}}Error(GenericError):
	pass
_lav.bindings_register_exception(_libaudioverse.{{error_name}}, {{friendly_name}}Error)

{%endfor%}

#A list-like thing that knows how to manipulate inputs.
class ParentProxy(collections.Sequence):
	"""Manipulate inputs for some specific object.
This works exactly like a python list, save that concatenation is not allowed.  The elements are tuples: (parent, output) or, should no parent be set for a slot, None.

To link a parent to an output using this object, use  obj.parents[num] = (myparent, output).
To clear a parent, assign None.

Note that these objects are always up to date with their associated libaudioverse object but that iterators to them will become outdated if anything changes the graph.

Note also that we are not inheriting from MutableSequence because we cannot support __del__ and insert, but that the above advertised functionality still works anyway."""

	def __init__(self, for_object):
		self.for_object = for_object	

	def __len__(self):
		return _lav.object_get_parent_count(self.for_object.handle)

	def __getitem__(self, key):
		par, out = self.for_object._get_parent(key)
		if par is None:
			return None
		return par, out

	def __setitem__(self, key, val):
		if len(val) != 2 and val is not None:
			raise TypeError("Expected list of length 2 or None.")
		if not isinstance(val[0], GenericObject):
			raise TypeError("val[0]: is not a Libaudioverse object.")
		self.for_object._set_parent(key, val[0] if val is not None else None, val[1] if val is not None else 0)

class _EventCallbackWrapper(object):
	"""Wraps callbacks into something sane.  Do not use externally."""

	def __init__(self, for_object, slot, callback, additional_args):
		self.obj_weakref = weakref.ref(for_object)
		self.additional_arguments = additional_args
		self.slot = slot
		self.callback = callback
		self.fptr = _libaudioverse.LavEventCallback(self)
		_lav.object_set_callback(for_object.handle, slot, self.fptr, None)

	def __call__(self, obj, userdata):
		actual_object = self.obj_weakref()
		if actual_object is None:
			return
		self.callback(actual_object, *self.additional_arguments)

class ArrayPropertyWrapper(collections.MutableSequence):
	"""Represents an array property.

Array properties have a minimum and maximum length which may be obtained with get_length_range.  In addition, all list operations work including appending.  An exception is thrown if any operation would cause the length to go outside the range returned by get_length_range."""

	def __init__(self, for_object, for_property, type_code):
		self.for_object = for_object
		self.for_property = for_property
		self.type_code = type_code
		if type_code == 0:
			self._length_query = _lav.object_get_int_array_property_length
			self._read = _lav.object_read_int_array_property
			self._write = _lav.object_write_int_array_property
			self._replace = _lav.object_replace_int_array_property
		elif type_code == 1:
			self._length_query = _lav.object_get_float_array_property_length
			self._read = _lav.object_raed_float_array_property
			self._write = _lav.object_write_float_array_property
			self._replace = _lav.object_replace_float_array_property

	def get_length_range(self):
		"""Returns a tuple (min, max).  The length of this array must never fall below min or rise above max."""
		return _lav.object_get_array_property_length_range()

	def __len__(self):
		return self._length_query(self.for_objject.handle, self.for_property)

	def __getitem__(self, index):
		if index < 0 or index >= len(self):
			raise IndexError()
		return self._read(self.for_object.handle, self.for_property, index)

	def __setitem__(self, index, value):
		if index < 0 or index >= len(self):
			raise IndexError()	
		if self.type_code == 0:
			value = int(value)
		elif sellf.type_code == 1:
			value = float(value)
		self._write(self.for_object.handle, self.for_property, index, index+1, value)

	def __delitem__(self, index):
		if index < 0 or index >= len(self):
			raise IndexError()
		l = list(self)
		del l[index]
		self._replace(self.for_object.handle, self.for_property, len(l), l)

	def insert(self, index, value):
		if index < 0 or index > len(self):
			raise IndexError()
		l = list(self)
		l.insert(index, int(value) if self.type_code == 0 else float(value))
		self._replace(self.for_object.handle, self.for_property, len(l), l)

class PhysicalOutput(object):
	"""Represents info on a physical output."""

	def __init__(self, latency, channels, name, index):
		self.latency = latency
		self.channels = channels
		self.name = name
		self.index = index

class Device(object):
	"""Represents an output, either to an audio card or otherwise.  A device is required by all other Libaudioverse objects."""


	def __init__(self, handle):
		self.handle = handle

	def get_block(self):
		"""Returns a block of data.
Calling this on an audio output device will cause the audio thread to skip ahead a block, so don't do that."""
		length = _lav.device_get_block_size(self.handle)*_lav.device_get_channels(self.handle)
		buff = (ctypes.c_float*length)()
		_lav.device_get_block(self.handle, buff)
		return list(buff)

	@staticmethod
	def get_physical_outputs():
		max_index = _lav.get_physical_output_count()
		outputs = []
		for i in xrange(max_index):
			info = PhysicalOutput(index = i,
			latency = _lav.get_physical_output_latency(i),
			channels = _lav.get_physical_output_channels(i),
			name = _lav.get_physical_output_name(i))
			outputs.append(info)
		return outputs

	@property
	def output_object(self):
		"""The object assigned to this property is the object which will play through the device."""
		return _wrap(_lav.device_get_output_object(self.handle))

	@output_object.setter
	def output_object(self, val):
		if not (isinstance(val, GenericObject) or val is None):
			raise TypeError("Expected subclass of Libaudioverse.GenericObject")
		_lav.device_set_output_object(self.handle, val.handle if val is not None else val)

#This is the class hierarchy.
#GenericObject is at the bottom, and we should never see one; and GenericObject should hold most implementation.
class GenericObject(object):
	"""A Libaudioverse object."""

	def __init__(self, handle):
		self.handle = handle
		self._parents = [(None, 0)]*_lav.object_get_parent_count(handle)
		self._callbacks = dict()

{%for enumerant, prop in metadata['Lav_OBJTYPE_GENERIC']['properties'].iteritems()%}
{{implement_property(enumerant, prop)}}
{%endfor%}
{%for enumerant, info in metadata['Lav_OBJTYPE_GENERIC'].get('callbacks', dict()).iteritems()%}
{{implement_callback(info['name'], "_libaudioverse." + enumerant)}}
{%endfor%}

	def __del__(self):
		if _lav is None:
			#undocumented python thing: if __del__ is called at process exit, globals of this module are None.
			return
		if getattr(self, 'handle', None) is not None:
			_lav.free(self.handle)
		self.handle = None

	@property
	def parents(self):
		"""Returns a ParentProxy, an object that acts like a list of tuples.  The first item of each tuple is the parent object and the second item is the output to which we are connected."""
		self._check_parent_resize()
		return ParentProxy(self)

	def _check_parent_resize(self):
		new_parent_count = _lav.object_get_parent_count(self.handle)
		new_parent_list = self._parents
		if new_parent_count < len(self._parents):
			new_parent_list = self._parents[0:new_parents_count]
		elif new_parent_count > len(self._parents):
			additional_parents = [(None, 0)]*(new_parent_count-len(self._parents))
			new_parent_list = new_parent_list + additional_parents
		self._parents = new_parent_list

	def _get_parent(self, key):
		return self._parents[key]

	def _set_parent(self, key, obj, inp):
		if obj is None:
			_lav.object_set_parent(self.handle, key, None, 0)
			self._parents[key] = (None, 0)
		else:
			_lav.object_set_parent(self.handle, key, obj.handle, inp)
			self._parents[key] = (obj, inp)

{%for object_name in constants.iterkeys()|prefix_filter("Lav_OBJTYPE_")|remove_filter("Lav_OBJTYPE_GENERIC")%}
{%set friendly_name = object_name|strip_prefix("Lav_OBJTYPE_")|lower|underscores_to_camelcase(True) + "Object"%}
{%set constructor_name = "Lav_create" + friendly_name%}
{%set constructor_arg_names = functions[constructor_name].input_args|map(attribute='name')|list-%}
class {{friendly_name}}(GenericObject):
	def __init__(self{%if constructor_arg_names|length > 0%}, {%endif%}{{constructor_arg_names|join(', ')}}):
		{{constructor_arg_names[0]}} = {{constructor_arg_names[0]}}.handle
		super({{friendly_name}}, self).__init__(_lav.{{constructor_name|without_lav|camelcase_to_underscores}}({{constructor_arg_names|join(', ')}}))

{%for enumerant, prop in metadata.get(object_name, dict()).get('properties', dict()).iteritems()%}
{{implement_property(enumerant, prop)}}

{%endfor%}
{%for enumerant, info in metadata.get(object_name, dict()).get('callbacks', dict()).iteritems()%}
{{implement_callback(info['name'], "_libaudioverse." + enumerant)}}

{%endfor%}
{%endfor%}

