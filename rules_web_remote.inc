<?php
// $Id$

/**
 * @file Rules Remote Sites - Include file.
 */

/**
 * Menu callback for notifying us of a remote event.
 */
function rules_web_remote_notify($remote) {
  if (($json = file_get_contents('php://input')) && $data = @drupal_json_decode($json)) {
    $events = $remote->events();
    if (!empty($data['event_name']) && !empty($data['event_data']) && isset($events[$data['event_name']])) {
      $name = 'rules_web_' . $remote->name . '_' . $data['event_name'];
      if ($event_set = rules_get_cache('event_' . $name)) {
        // Make sure passed arguments are complete and invoke event.
        $event_args = $event_set->argumentInfo();
        if (($args = array_intersect_key((array)$data['event_data'], $event_args)) && count($args) == count($event_args)) {
          $event_set->execute($args);
          $t_args = array('%event' => $data['event_name'], '%remote' => $remote->label, '%log' => RulesLog::logger()->render());
          watchdog('rules web remote', 'Event %event of remote %name triggered rule evaluation: %log', $t_args, WATCHDOG_NOTICE);
        }
      }
      else {
        // We don't have any configured rules, so unsubscribe.
        $remote->unsubscribe($data['event_name']);
      }
    }
    elseif (isset($data['handshake'])) {
      // Accept handshakes to verify that this is a valid callback.
      return TRUE;
    }
    else {
      drupal_add_http_header('404 Not Found');
    }
  }
  else {
    drupal_add_http_header('415 Unsupported Media Type');
  }
}

/**
 * Class representing remote sites.
 */
class RulesWebRemote extends EntityDB {

  public $settings = array();
  public $cache = array();
  public $name;
  public $label;
  public $url;

  protected $endpoint = NULL;

  function __construct($values = array(), $entity_type = 'rules_web_remote') {
    parent::__construct($values, $entity_type);
  }

  public function access($account = NULL) {
    if (method_exists($this->endpoint(), 'access')) {
      return $this->endpoint()->access($account);
    }
    return user_access('interact with remote ' . $this->name, $account);
  }

  /**
   * @return RulesWebRemoteEndpointInterface
   */
  protected function endpoint() {
    if (!isset($this->endpoint)) {
      $types = rules_web_remote_get_types();
      $this->endpoint = new $types[$this->type]['class']($this);
    }
    return $this->endpoint;
  }

  /**
   * Loads the data of the given type and id. In case of errors a RulesException
   * is thrown.
   */
  public function load($type, $id) {
    try {
      return $this->endpoint()->load($type, $id);
    }
    catch (Exception $e) {
      $args = array('%name' => $this->name, '%message' => $e->getMessage(), '%type' => $type, '%id' => $id);
      throw new RulesException('Unable to load %id of type %type from the remote site %name. Error message: %message', $args, RulesLog::WARN);
    }
  }

  public function events() {
    if (!isset($this->cache['events'])) {
      $this->refreshCache();
    }
    return !empty($this->cache['events']) ? $this->cache['events'] : array();
  }

  public function entities() {
    if (!isset($this->cache['entities'])) {
      $this->refreshCache();
    }
    return !empty($this->cache['entities']) ? $this->cache['entities'] : array();
  }

  public function actions() {
    if (!isset($this->cache['actions'])) {
      $this->refreshCache();
    }
    return !empty($this->cache['actions']) ? $this->cache['actions'] : array();
  }

  public function conditions() {
    if (!isset($this->cache['conditions'])) {
      $this->refreshCache();
    }
    return !empty($this->cache['conditions']) ? $this->cache['conditions'] : array();
  }

  protected function refreshCache() {
    try {
      $this->cache['entities'] = $this->endpoint()->entities();
      $this->cache['actions'] = $this->endpoint()->actions();
      $this->cache['conditions'] = $this->endpoint()->conditions();
      $this->cache['events'] = $this->endpoint()->events();
    }
    catch (Exception $e) {
      $args = array('%name' => $this->name, '%message' => $e->getMessage());
      watchdog('rules remote', 'Error getting definitions from the remote site %name. Error message: %message.', $args, WATCHDOG_ALERT);
    }
  }

  public function clearCache() {
    $this->cache = array();
  }

  public function subscribe($event) {
    try {
      $this->endpoint()->subscribe($event);
    }
    catch (Exception $e) {
      $args = array('%event' => $event, '%name' => $this->name, '%message' => $e->getMessage());
      watchdog('rules remote', 'Error subscribing to event %event of remote site %name. Error message: %message.', $args, WATCHDOG_ERROR);
    }
  }

  public function unsubscribe($event) {
    try {
      $this->endpoint()->unsubscribe($event);
    }
    catch (Exception $e) {
      $args = array('%event' => $event, '%name' => $this->name, '%message' => $e->getMessage());
      watchdog('rules remote', 'Error unsubscribing from event %event of remote site %name. Error message: %message.', $args, WATCHDOG_ERROR);
    }
  }
}


/**
 * Interface for remote endpoints. In case of any errors the implementing
 * classes should throw exceptions.
 */
interface RulesWebRemoteEndpointInterface {

  /**
   * Load remote data.
   */
  public function load($type, $id);

  /**
   * An array of definitions for the provided events.
   */
  public function events();

  /**
   * Subscribe to a remote event.
   */
  public function subscribe($event);

  /**
   * Unsubscribe from a remote event.
   */
  public function unsubscribe($event);

  /**
   * An array of info about entity types used by the provided
   * events/conditions/actions.
   */
  public function entities();

  /**
   * An array of definitions for the provided actions.
   */
  public function actions();

  /**
   * An array of definitions for the provided conditions.
   */
  public function conditions();
}

/**
 * A remote endpoint types for rules web hooks.
 */
class RulesWebRemoteEndpointWebHooks implements RulesWebRemoteEndpointInterface {

  /**
   * @var RulesWebRemote
   */
  protected $remote, $url;

  /**
   * @var RestClient
   */
  protected  $client;

  public function __construct($remote, $base_path = 'rules_web') {
    $this->remote = $remote;
    $this->url = $remote->url . '/' . $base_path . '/';
  }

  public function client() {
    if (!isset($this->client)) {
      $this->client = new RestClient(NULL, new RestClientBaseFormatter(RestClientBaseFormatter::FORMAT_JSON));
      // Pass through additional curl options.
      if (!empty($this->remote->settings['curl options'])) {
        $this->client->curlOpts = $this->remote->settings['curl options'];
      }
    }
    return $this->client;
  }

  public function load($type, $id) {
    if (valid_url($url = $this->url . "$type/$id.json", TRUE)) {
      return (object)$this->client()->get($url);
    }
  }

  public function events() {
    return $this->client()->get($this->url . "rules_web_hook.json");
  }

  public function subscribe($event) {
    if (valid_url($url = $this->url . "rules_web_hook/$event/subscribe.json", TRUE)) {
      $this->client()->post($url, array(
         'url' => url('rules_web/rules_web_remote/' . $this->remote->name . '/notify', array('absolute' => TRUE)),
         'http_auth' => rules_web_remote_get_http_auth(),
      ));
    }
  }

  public function unsubscribe($event) {
    if (valid_url($url = $this->url . "rules_web_hook/$event/unsubscribe.json", TRUE)) {
      $this->client()->post($url, array(
         'url' => url('rules_web/rules_web_remote/' . $this->remote->name . '/notify', array('absolute' => TRUE)),
      ));
    }
  }

  public function entities() {
    return $this->client()->get($this->url . 'entity_metadata.json');
  }

  public function actions() {
    return array();
  }

  public function conditions() {
    return array();
  }
}

/**
 * A controller for loading remote data.
 */
class RulesWebRemoteEntityController extends DrupalDefaultEntityController {

  protected $remote;

  function __construct($entityType) {
    parent::__construct($entityType);
    $this->remote = rules_web_remote_load($this->entityInfo['rules web remote']['remote']);
  }


  /**
   * Override load to fetch the data from the remote site. For now we don't
   * support using $conditions or revisions.
   */
  public function load($ids = array(), $conditions = array()) {
    $this->ids = $ids;
    $this->conditions = $conditions;

    $entities = array();

    // Create a new variable which is either a prepared version of the $ids
    // array for later comparison with the entity cache, or FALSE if no $ids
    // were passed. The $ids array is reduced as items are loaded from cache,
    // and we need to know if it's empty for this reason to avoid querying the
    // database when all requested entities are loaded from cache.
    $passed_ids = !empty($this->ids) ? array_flip($this->ids) : FALSE;
    // Try to load entities from the static cache, if the entity type supports
    // static caching.
    if ($this->cache) {
      $entities += $this->cacheGet($this->ids, $this->conditions);
      // If any entities were loaded, remove them from the ids still to load.
      if ($passed_ids) {
        $this->ids = array_keys(array_diff_key($passed_ids, $entities));
      }
    }

    // Load any remaining entities from the remote site.
    if ($this->ids === FALSE || $this->ids) {
      foreach ($this->ids as $id) {
        $queried_entities[$id] = $this->remote->load($this->entityInfo['rules web remote']['type'], $id);
      }
    }

    // Pass all entities loaded from the database through $this->attachLoad(),
    // which attaches fields (if supported by the entity type) and calls the
    // entity type specific load callback, for example hook_node_load().
    if (!empty($queried_entities)) {
      $this->attachLoad($queried_entities);
      $entities += $queried_entities;
    }

    if ($this->cache && !empty($queried_entities)) {
      $this->cacheSet($queried_entities);
    }

    // Ensure that the returned array is ordered the same as the original
    // $ids array if this was passed in and remove any invalid ids.
    if ($passed_ids) {
      // Remove any invalid ids from the array.
      $passed_ids = array_intersect_key($passed_ids, $entities);
      foreach ($entities as $entity) {
        $passed_ids[$entity->{$this->idKey}] = $entity;
      }
      $entities = $passed_ids;
    }

    return $entities;
  }
}

