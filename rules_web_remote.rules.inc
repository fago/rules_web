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
  $valid_types = rules_fetch_data('data_info');

  foreach (entity_load('rules_web_remote', FALSE) as $name => $remote) {
    $remote_types = $remote->entities() + $remote->dataTypes();
    foreach ($remote->events() as $event => $event_info) {
      $is_valid = array_diff_key(array_flip(array('label', 'variables')), $event_info) == array();
      // Don't add in events, that stem from another site.
      if (strpos($event, 'rules_web_') !== 0 && $is_valid) {
        // Transform data types of the variables
        foreach ($event_info['variables'] as $var_name => &$var_info) {
          // Make sure the info is complete and contains known types.
          if (array_diff_key(array_flip(array('label', 'type')), $var_info) !== array()) {
            continue 2;
          }
          // Map the type name of types defined by the remote.
          $var_info['type'] = _rules_web_remote_map_type($name, $remote_types, $var_info['type']);

          if (!isset($valid_types[$var_info['type']])) {
            continue 2;
          }
          // We don't support loading via handler for remote variables.
          unset($var_info['handler']);
        }
        $event_info['group'] = t('Remote @label', array('@label' => $remote->label));
        // Add some info so we can easily get the remote for an event later on.
        $event_info['rules web remote']['remote'] = $name;
        $event_info['rules web remote']['event'] = $event;
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
      $type_info = array_intersect_key($type_info, array_flip(array('label', 'property info'))) + array(
        'wrap' => TRUE,
        'label' => t('Remote %label %type', array('%label' => $remote->label, '%type' => $type)),
      );
      $types['rules_web_' . $name . '_' . $type] = $type_info;
    }
  }
  return $types;
}

/**
 * Implements hook_rules_action_info().
 */
function rules_web_remote_rules_action_info() {
  $return = array();
  foreach (entity_load('rules_web_remote', FALSE) as $name => $remote) {
    $remote_types = $remote->entities() + $remote->dataTypes();
    foreach ($remote->actions() as $item_name => $info) {
      $info += array('parameter' => array(), 'provides' => array());
      // Map the types.
      foreach ($info['parameter'] as $param => &$param_info) {
        $param_info['type'] = _rules_web_remote_map_type($name, $remote_types, $param_info['type']);
      }
      foreach ($info['provides'] as $var_name => &$var_info) {
        $var_info['type'] = _rules_web_remote_map_type($name, $remote_types, $var_info['type']);
      }
      $return['rules_web_' . $name . '_' . $item_name] = $info;
    }
  }
  return $return;
}

/**
 * Implements hook_rules_condition_info().
 */
function rules_web_remote_rules_condition_info() {
  $return = array();
  foreach (entity_load('rules_web_remote', FALSE) as $name => $remote) {
    $remote_types = $remote->entities() + $remote->dataTypes();
    foreach ($remote->conditions() as $item_name => $info) {
      $info += array('parameter' => array());
      // Map the types.
      foreach ($info['parameter'] as $param => &$param_info) {
        $param_info['type'] = _rules_web_remote_map_type($name, $remote_types, $param_info['type']);
      }
      $return['rules_web_' . $name . '_' . $item_name] = $info;
    }
  }
  return $return;
}

/**
 * Maps the type name from the name used by the remote info to the rules name.
 */
function _rules_web_remote_map_type($name, $remote_types, $type) {
  if (is_array($type)) {
    foreach ($type as $key => $entry) {
      $type[$key] = _rules_web_remote_map_type($name, $remote_types, $entry);
    }
  }
  elseif (isset($remote_types[$type])) {
    $type = 'rules_web_' . $name . '_' . $type;
  }
  return $type;
}

