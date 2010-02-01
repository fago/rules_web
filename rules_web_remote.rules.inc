<?php
// $Id$

/**
 * @file Rules Remote Sites - Rules integration.
 */


/**
 * Implements hook_rules_event_info().
 */
function rules_web_remote_rules_event_info() {
  $events = array();
  $cache = rules_get_cache();
  $valid_types = $cache['data_info'] + entity_get_info();

  foreach (entity_load('rules_web_remote', FALSE) as $name => $remote) {
    $remote_types = $remote->dataTypes();
    foreach ($remote->events() as $event => $event_info) {
      // Don't add in events, that stem from another site.
      $is_valid = array_diff_key(array_flip(array('label', 'variables')), $event_info) == array();
      if (strpos($event, 'rules_web_') !== 0 && $is_valid) {
        // Transform data types of the variables
        foreach ($event_info['variables'] as $var_name => &$var_info) {
          // Make sure the info is complete and contains known types.
          if (array_diff_key(array_flip(array('label', 'type')), $var_info) !== array()) {
            continue 2;
          }
          // Map the type name of types defined by the remote.
          if (isset($remote_types[$var_info['type']])) {
            $var_info['type'] = 'rules_web_' . $name . '_' . $var_info['type'];
          }
          if (!isset($valid_types[$var_info['type']])) {
            continue 2;
          }
          // We don't support loading via handler for remote variables.
          unset($var_info['handler']);
        }
        $events['rules_web_' . $name . '_' . $event] = $event_info;
      }
    }
  }
  return $events;
}

/**
 * Implements hook_rules_data_info().
 */
function rules_web_remote_rules_data_info() {
  $types = array();
  foreach (entity_load('rules_web_remote', FALSE) as $name => $remote) {
    foreach ($remote->dataTypes() as $type => $type_info) {
      // Don't add in entity types, that stem from another site.
      if (strpos($type, 'rules_web_') !== 0) {
        $label = isset($type_info['label']) ? $type_info['label'] : $type;
        $types['rules_web_' . $name . '_' . $type] = array(
          'label' => t('Remote %name %type', array('%name' => $remote->label, '%type' => $type)),
          'class' => 'RulesDataEntityWrapper',
          'parent' => 'entity',
        );
      }
    }
  }
  return $types;
}
