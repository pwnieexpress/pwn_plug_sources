extern struct bluetooth_plugin_desc __bluetooth_builtin_audio;
extern struct bluetooth_plugin_desc __bluetooth_builtin_input;
extern struct bluetooth_plugin_desc __bluetooth_builtin_serial;
extern struct bluetooth_plugin_desc __bluetooth_builtin_network;
extern struct bluetooth_plugin_desc __bluetooth_builtin_service;
extern struct bluetooth_plugin_desc __bluetooth_builtin_hciops;
extern struct bluetooth_plugin_desc __bluetooth_builtin_mgmtops;
extern struct bluetooth_plugin_desc __bluetooth_builtin_formfactor;
extern struct bluetooth_plugin_desc __bluetooth_builtin_storage;
extern struct bluetooth_plugin_desc __bluetooth_builtin_adaptername;

static struct bluetooth_plugin_desc *__bluetooth_builtin[] = {
  &__bluetooth_builtin_audio,
  &__bluetooth_builtin_input,
  &__bluetooth_builtin_serial,
  &__bluetooth_builtin_network,
  &__bluetooth_builtin_service,
  &__bluetooth_builtin_hciops,
  &__bluetooth_builtin_mgmtops,
  &__bluetooth_builtin_formfactor,
  &__bluetooth_builtin_storage,
  &__bluetooth_builtin_adaptername,
  NULL
};
