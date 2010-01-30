<?php
// $Id$

/**
 * @file Rules Remote Sites - Include file.
 */

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
      try {
        $this->cache['events'] = $this->endpoint()->events();
      }
      catch (Exception $e) {
        $args = array('%name' => $this->name, '%message' => $e->getMessage());
        watchdog('rules remote', 'Error getting web hooks from the remote site %name. Error message: %message.', $args, WATCHDOG_ALERT);
        return array();
      }
    }
    return $this->cache['events'];
  }
  
  public function dataTypes() {
    if (!isset($this->cache['data types'])) {
      try {
        $this->cache['data types'] = $this->endpoint()->dataTypes();
      }
      catch (Exception $e) {
        $args = array('%name' => $this->name, '%message' => $e->getMessage());
        watchdog('rules remote', 'Error getting the data types from the remote site %name. Error message: %message.', $args, WATCHDOG_ALERT);
        return array();
      }
    }
    return $this->cache['data types'];
  }

  //TODO: Allow providing actions.
  public function actions() {
    return $this->endpoint()->actions();
  }

  //TODO: Allow providing conditions.
  public function conditions() {
    return $this->endpoint()->conditions();
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
   * An array of data types used by the provided events/conditions/actions.
   */
  public function dataTypes();
  
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
  
  protected function client() {
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
    return (object)$this->client()->get($this->url . "$type/$id.json", array());
  }
  
  public function events() {
    return $this->client()->get($this->url . "rules_web_hook.json", array());
  }
  
  public function dataTypes() {
    return $this->client()->get($this->url . 'entity_metadata.json', array());
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

